/*
 * crane-domain.h by @nieltg
 */

#ifndef __CRANE_DOMAIN_H__
#define __CRANE_DOMAIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CRANE_TYPE_DOMAIN               (crane_domain_get_type())
#define CRANE_DOMAIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),CRANE_TYPE_DOMAIN,CraneDomain))
#define CRANE_DOMAIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),CRANE_TYPE_DOMAIN,CraneDomainClass))
#define CRANE_IS_DOMAIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),CRANE_TYPE_DOMAIN))
#define CRANE_IS_DOMAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),CRANE_TYPE_DOMAIN))
#define CRANE_DOMAIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj),CRANE_TYPE_DOMAIN,CraneDomainClass))

typedef struct _CraneDomain             CraneDomain;
typedef struct _CraneDomainClass        CraneDomainClass;
typedef struct _CraneDomainPrivate      CraneDomainPrivate;

struct _CraneDomain
{
	GObject parent;
	
	CraneDomainPrivate *_priv;
};

struct _CraneDomainClass
{
	GObjectClass parent_class;
	
	void	(* bundle_installed)    (CraneDomain * domain,
	                                 CraneBundle * bundle);
	void	(* bundle_removed)      (CraneDomain * domain,
	                                 CraneBundle * bundle);
	void	(* bundle_started)      (CraneDomain * domain,
	                                 CraneBundle * bundle);
	void	(* bundle_stopped)      (CraneDomain * domain,
	                                 CraneBundle * bundle);
};
                                                     
GType           crane_domain_get_type   (void) G_GNUC_CONST;

CraneDomain*    crane_domain_new        (void);

void            crane_domain_acquire    (CraneDomain * self, GFile * dir, GError ** error);
void            crane_domain_release    (CraneDomain * self, GError ** error) ;

G_END_DECLS

#endif /* __CRANE_DOMAIN_H__ */
