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
	PROP_PATH,
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
	case PROP_PATH:
		g_value_dup_string (value,
		                    self->_priv->path);
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
		g_param_spec_string ("path",
		                     "Path",
		                     "Path to domain",
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
	
	obj->_priv->path = NULL;
	obj->_priv->bundles = NULL;
	
	obj->_priv->is_acquired = FALSE;
}

CraneDomain *
crane_domain_new (void)
{
	return g_object_new (CRANE_TYPE_DOMAIN, NULL);
}

gchar *
crane_domain_get_path (CraneDomain * self)
{
	g_return_val_if_fail (CRANE_IS_DOMAIN (self), FALSE);
	
	return g_strdup (self->_priv->path);
}

gboolean
crane_domain_is_acquired (CraneDomain * self)
{
	g_return_val_if_fail (CRANE_IS_DOMAIN (self), FALSE);
	
	return self->_priv->path != NULL;
}

void
crane_domain_acquire (CraneDomain * self, gchar * path, GError ** error)
{
	g_return_if_fail (CRANE_IS_DOMAIN (self));
	g_return_if_fail (!crane_domain_is_acquired (self));
	
	char * root_path = g_strdup (path);
	
	/* Check for availability by PID file. */
	
	char * pid_path = g_build_filename (root_path, ".pidfile");
	
	errno = 0;
	
	FILE * pid_h_r = fopen (pid_path, "r");
	int err_open_r = errno;
	
	if (pid_h_r != NULL)
	{
		/* PID file exists and can be read. */
		
		int pid;
		
		int cnt = fscanf (pid_h_r, "%d", &pid);
		int err_scan = errno;
		
		if (cnt > 0)
		{
			/* PID file is not broken and contain a PID number. */
			
			char * proc = g_build_filename ("/proc", itoa (pid));
			
			if (g_file_test (proc, G_FILE_TEST_EXISTS))
			{
				/* PID is valid. Domain is used by another process. */
				
				fclose (pid_h_r);
				
				g_free (pid_path);
				g_free (root_path);
				
				g_error_set_literal (error,
				                     G_FILE_ERROR,
				                     G_FILE_ERROR_FAILED,
				                     "Domain has acquired");
				return;
			}
			else
			{
				/* PID is invalid. Process using domain maybe killed. */
			}
		}
		else
		{
			/* Unable to read PID number from PID file. */
			
			if (ferror (pid_h_r))
			{
				/* An error has caused reading operation failed. */
				
				fclose (pid_h_r);
				
				g_free (pid_path);
				g_free (root_path);
				
				g_error_set (error,
				             G_FILE_ERROR,
				             g_file_error_from_errno (err_scan),
				             "Unable to read PID file: %s",
				             strerror (err_scan));
				return;
			}
			else
			{
				/* PID file is malformed. Ignore and continue. */
			}
		}
		
		fclose (pid_h_r);
		pid_h_r = NULL;
	}
	else
	{
		/* Error is occured while opening PID file for reading. */
		
		if (err_open_r == ENOENT)
		{
			/* PID file is not exist. Domain is free. Continue. */
		}
		else
		{
			/* Another error has caused reading operation failed. */
			
			g_free (pid_path);
			g_free (root_path);
			
			g_error_set (error,
			             G_FILE_ERROR,
			             g_file_error_from_errno (err_open_r),
			             "Unable to open PID file for reading: %s",
			             strerror (err_open_r));
			return;
		}
	}
	
	/* Write PID file to the domain. */
	
	errno = 0;
	
	FILE * pid_h_w = fopen (pid_path, "w");
	int err_open_w = errno;
	
	g_clear_pointer (&pid_path, g_free);
	
	if (pid_h_r != NULL)
	{
		/* PID file can be opened for writing. */
		
		int ret_print = fprintf (pid_h_w, "%d\n", getpid ());
		int err_print = errno;
		
		if (ret_print < 0)
		{
			/* An error has caused writing operation failed. */
			
			g_free (root_path);
			
			g_error_set (error,
			             G_FILE_ERROR,
			             g_file_error_from_errno (err_print),
			             "Unable to write PID file: %s",
			             strerror (err_print));
			return;
		}
		
		int ret_flush = fflush (pid_h_w);
		int err_flush = errno;
		
		if (ret_flush != 0)
		{
			/* An error has caused flushing operation failed. */
			
			g_free (root_path);
			
			g_error_set (error,
			             G_FILE_ERROR,
			             g_file_error_from_errno (err_flush),
			             "Unable to flush PID file: %s",
			             strerror (err_flush));
			return;
		}
		
		fclose (pid_h_w);
		pid_h_w = NULL;
	}
	else
	{
		/* Error is occured while opening PID file for writing. */
		
		g_free (root_path);
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_open_w),
		             "Unable to open PID file for writing: %s",
		             strerror (err_open_w));
		return;
	}
	
	// TODO: here!
	
	// check directory structure, if malformed, error.
	// read apps list.
	
	self->_priv->path = root_path;
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
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
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_ACQUIRED]);
}

