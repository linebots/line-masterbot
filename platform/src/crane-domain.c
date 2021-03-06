/*
 * crane-domain.c by @nieltg
 */

#include "crane-domain.h"

struct _CraneDomainPrivate
{
	gchar * path;
	int pid_fd;
	
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
	
	g_clear_pointer (&self->_priv->bundles, g_hash_table_unref);
	
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
release_bundle_tool (gpointer * bundle)
{
	g_return_if_fail (CRANE_IS_BUNDLE (bundle));
	
	CraneBundle * bundle = CRANE_BUNDLE (bundle);
	crane_bundle_release (bundle);
	
	g_object_unref (G_OBJECT (bundle));
}

static void
crane_domain_init (CraneDomain * obj)
{
	obj->_priv = crane_domain_get_instance_private (obj);
	
	obj->_priv->path = NULL;
	obj->_priv->pid_fd = -1;
	
	obj->_priv->bundles = g_hash_table_new_full (g_str_hash,
	                                             g_str_equal,
	                                             g_free,
	                                             release_bundle_tool);
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
	
	return self->_priv->pid_fd != -1;
}

void
crane_domain_acquire (CraneDomain * self, gchar * path, GError ** error)
{
	g_return_if_fail (CRANE_IS_DOMAIN (self));
	g_return_if_fail (!crane_domain_is_acquired (self));
	
	char * root_path = g_strdup (path);
	
	int ret;
	int err_code;
	GError * g_err;
	
	/* Check for availability by PID file. */
	
	char * path_pid = g_build_filename (root_path, ".pidlock");
	
	errno = 0;
	
	int fd = open (path_pid, O_WRONLY | O_CREAT | O_SYNC, 0644);
	err_code = errno;
	
	g_clear_pointer (&path_pid, g_free);
	
	if (fd == -1)
	{
		/* An error has caused the operation to be failed. */
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_code),
		             "Unable to open pid-file: %s",
		             strerror (err_code));
		goto error_1;
	}
	
	ret = flock (fd, LOCK_EX | LOCK_NB);
	err_code = errno;
	
	if (ret == -1)
	{
		/* Unable to lock. Domain maybe used by another process. */
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_code),
		             "Unable to lock pid-file: %s",
		             strerror (err_code));
		goto error_2;
	}
	
	/* Write PID file to the domain. */
	
	ret = ftruncate (fd, 0);
	err_code = errno;
	
	if (ret == -1)
	{
		/* Unable to truncate PID file. */
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_code),
		             "Unable to truncate pid-file: %s",
		             strerror (err_code));
		goto error_2;
	}
	
	ret = dprintf (fd, "%d\n", getpid ());
	err_code = errno;
	
	if (ret < 0)
	{
		/* Unable to write current PID to PID file. */
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_code),
		             "Unable to write pid-file: %s",
		             strerror (err_code));
		goto error_2;
	}
	
	/* Generate apps list kept inside the bundle. */
	
	char * path_app = g_build_filename (root_path, "app");
	
	g_err = NULL;
	GDir * dir_app = g_dir_open (path_app, 0, &g_err);
	
	g_clear_pointer (&path_app, g_free);
	
	if (dir_app == NULL)
	{
		/* Error is occured while opening apps directory. */
		
		g_propagate_error (error, g_err);
		goto error_2;
	}
	
	errno = 0;
	
	while ((const gchar * en_raw = g_dir_read_name (dir_app)) != NULL)
	{
		gchar * en = g_strdup (en_raw);
		
		if (!g_hash_table_insert (self->_priv->bundles, en, NULL))
		{
			/* Key already exist. Duplicate bundle-id detected! */
			/* NOTE: 'en' is freed by GHashTable's remove-hook. */
			
			g_error_set_literal (error,
			                     G_FILE_ERROR,
			                     G_FILE_ERROR_FAILED,
			                     "Domain is broken: Encountered duplicate bundle-id");
			goto error_3;
		}
		
		CraneBundle * tool = crane_bundle_new ();
		
		g_err = NULL;
		ret = crane_bundle_acquire (tool, self, en_raw, g_err); 
		
		g_object_unref (G_OBJECT (tool));
		
		if (!ret)
		{
			/* Unable to process a bundle. Domain is broken! */
			
			g_propagate_error (error, g_err);
			goto error_3;
		}
	}
	
	if (errno != 0)
	{
		/* An error has occured while enumerating directory. */
		
		g_error_set (error,
		             G_FILE_ERROR,
		             g_file_error_from_errno (err_code),
		             "Directory enumeration failure: %s",
		             strerror (err_code));
		goto error_3;
	}
	
	g_clear_pointer (&dir_app, g_dir_close);
	
	self->_priv->pid_fd = fd;
	self->_priv->path = root_path;
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_ACQUIRED]);
	
	// TODO: generate acquired signal!
	
	return;
	
error_3:
	g_hash_table_remove_all (self->_priv->bundles);
	g_dir_close (dir_app);
error_2:
	close (fd);
error_1:
	g_free (root_path);
}

void
crane_domain_release (CraneDomain * self)
{
	g_return_if_fail (CRANE_IS_DOMAIN (self));
	g_return_if_fail (crane_domain_is_acquired (self));
	
	// TODO: generate releasing signal!
	
	/* Release all bundles from bundle-tools. */
	
	g_hash_table_remove_all (self->_priv->bundles);
	
	close (fd);
	
	self->_priv->pid_fd = -1;
	g_clear_pointer (&self->_priv->path, g_free);
	
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_ACQUIRED]);
}

