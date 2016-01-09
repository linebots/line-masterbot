/*
 * crane-domain.c by @nieltg
 */

#include "crane-domain.h"

struct _CraneDomainPrivate
{
	gchar * path;
	gboolean is_acquired;
	
	GList * bundles;        // deprecated!
};

/*
 * NOTE: CraneDomainPrivate.bundles is deprecated!
 *
 * There IS NOT ANY bundle-obj, but there IS bundle-tool.
 * Bundle-tool can be instanced by domain or user, but to be usable,
 * it should be associated with domain. Domain will ONLY create bundle-
 * tool if it's needed. Associated bundle-tool is registered, another
 * bundle-tool can't register with same bundle-id in a domain.
 * 
 * void crane_bundle_handle (CraneBundle * self, CraneDomain * domain, gchar * bundle_id);
 * boolean _crane_domain_associate (CraneDomain * self, CraneBundle * self);
 * 
 * - crane_bundle_handle sets "domain" and "bundle-id" properties.
 * - crane_bundle_handle calls _crane_domain_associate to get GFile.
 * - _crane_domain_associate stores CraneBundle uniquely by bundle-id, if fail, return null.
 * - crane_bundle_handle set "file" prop (if can) and return.
 */

enum
{
	PROP_0,
	PROP_FILE,
	PROP_IS_ACQUIRED,
	LAST_PROP
}

static GParamSpec * properties[LAST_PROP];

enum
{
	BUNDLE_INSTALLED,
	BUNDLE_REMOVED,
	BUNDLE_STARTED,
	BUNDLE_STOPPED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (CraneDomain, crane_domain, G_TYPE_OBJECT)

static void
crane_domain_get_property (GObject    * object,
                           guint        prop_id,
                           GValue     * value,
                           GParamSpec * pspec)
{
	CraneDomain * self = CRANE_DOMAIN (object);
	
	switch (prop_id)
	{
	case PROP_FILE:
		g_value_set_object (value,
		                    self->_priv->file);
		break;
	
	case PROP_IS_ACQUIRED:
		g_value_set_boolean (value,
		                     crane_domain_is_acquired (self));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
crane_domain_set_property (GObject    * object,
                           guint        prop_id,
                           GValue     * value,
                           GParamSpec * pspec)
{
	CraneDomain * self = CRANE_DOMAIN (object);
	
	switch (prop_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
crane_domain_dispose (GObject * obj)
{
	CraneDomain * self = CRANE_DOMAIN (obj);
	
	crane_domain_release (self, NULL);
	g_clear_object (&self->_priv->file);
	
	G_OBJECT_CLASS (crane_domain_parent_class)->dispose (obj);
}

static void
crane_domain_finalize (GObject * obj)
{
	CraneDomain * self = CRANE_DOMAIN (obj);
	
	/* free instance resources here */
	G_OBJECT_CLASS (crane_domain_parent_class)->finalize (obj);
}

static void
crane_domain_class_init (CraneDomainClass * klass)
{
	GObjectClass * object_class = (GObjectClass *) klass;
	
	object_class->dispose = crane_domain_dispose;
	object_class->finalize = crane_domain_finalize;
	object_class->get_property = crane_domain_get_property;
	object_class->set_property = crane_domain_set_property;
	
	properties[PROP_PATH] =
		g_param_spec_string ("file",
		                     "File",
		                     "Filesystem representation of domain",
		                     NULL,
		                     G_PARAM_READONLY | G_PARAM_STATIC_STRINGS);
	properties[PROP_IS_ACQUIRED] =
		g_param_spec_boolean ("is-acquired",
		                      "Is Acquired",
		                      "Is domain acquired",
		                      NULL,
		                      G_PARAM_READONLY | G_PARAM_STATIC_STRINGS);
	
	g_object_class_install_properties (object_class, LAST_PROP, properties);
	
	signals[BUNDLE_INSTALLED] =
		g_signal_new ("bundle-installed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (CraneDomainClass, bundle_installed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      CRANE_TYPE_DOMAIN,
			      CRANE_TYPE_BUNDLE);
	signals[BUNDLE_REMOVED] =
		g_signal_new ("bundle-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (CraneDomainClass, bundle_removed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      CRANE_TYPE_DOMAIN,
			      CRANE_TYPE_BUNDLE);
	signals[BUNDLE_STARTED] =
		g_signal_new ("bundle-started",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (CraneDomainClass, bundle_started),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      CRANE_TYPE_DOMAIN,
			      CRANE_TYPE_BUNDLE);
	signals[BUNDLE_STOPPED] =
		g_signal_new ("bundle-stopped",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (CraneDomainClass, bundle_stopped),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      CRANE_TYPE_DOMAIN,
			      CRANE_TYPE_BUNDLE);
}

static void
crane_domain_init (CraneDomain * obj)
{
	obj->_priv = crane_domain_get_instance_private (obj);
	
	obj->_priv->file = NULL;
	obj->_priv->bundles = NULL;
	
	obj->_priv->is_acquired = FALSE;
}

CraneDomain *
crane_domain_new (void)
{
	return g_object_new (CRANE_TYPE_DOMAIN, NULL);
}

gboolean
crane_domain_is_acquired (CraneDomain * self)
{
	g_return_val_if_fail (CRANE_IS_DOMAIN (self), FALSE);
	
	return self->_priv->file != NULL;
}

void
crane_domain_acquire (CraneDomain * self, GFile * dir, GError ** error)
{
	g_return_if_fail (CRANE_IS_DOMAIN (self));
	g_return_if_fail (!crane_domain_is_acquired (self));
	
	/* Check for native path availability. */
	
	gchar * path = g_file_get_path (dir);
	g_return_if_fail (path != NULL);
	
	g_free (path);
	
	/* Check for availability of PID file. */
	
	GFile * pid_file = g_file_get_child (dir, ".pidfile");
	
	GError * error = NULL;
	char ** pidcon = NULL;
	gsize len = 0;
	
	gboolean stat = g_file_load_contents (pid_file,
	                                      NULL, &pidcon, &len,
	                                      NULL, &error);
	
	if (stat)
	{
		int pid;
		int cnt = sscanf (pidcon, "%d", &pid);
		
		if (cnt > 0)
		{
			
		}
		
		// else, malformed pid file, continue.
	}
	else
	{
		// if error != (file not found), propragate error!
	}
	
	// TODO: here!
	
	// write pid file, if fail, error.
	// check directory structure, if malformed, error.
	// read apps list.
	
	g_object_ref (G_OBJECT (dir));
	self->_priv->file = dir;
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_ACQUIRED]);
}

void
crane_domain_release (CraneDomain * self, GError ** error)
{
	g_return_if_fail (CRANE_IS_DOMAIN (self));
	
	// TODO: here!
	// if not acquired, just return without error.
	// send "disposing" signal, every bundle-tools should clean-up, then stop.
	// unref every bundle-tools registered to this domain.
	// remove pid file, if fail, error.
	// update properties.
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_ACQUIRED]);
}

