#ifndef __RCV_TUNER_H__
#define __RCV_TUNER_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_RCV_TUNER               (rcv_tuner_get_type ())
#define RCV_TUNER(obj)               (GTK_CHECK_CAST (obj, TYPE_RCV_TUNER, RcvTuner))
#define RCV_TUNER_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_RCV_TUNER, RcvTunerClass))
#define IS_RCV_TUNER(obj)            (GTK_CHECK_TYPE (obj, TYPE_RCV_TUNER))
#define IS_RCV_TUNER_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_RCV_TUNER))

#ifdef GTK2
#define RCV_TUNER_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_RCV_TUNER, RcvTunerClass))
#else
#define RCV_TUNER_GET_CLASS(obj)     (RCV_TUNER_CLASS (obj->klass))
#endif

typedef struct _RcvTuner       RcvTuner;
typedef struct _RcvTunerClass  RcvTunerClass;

struct _RcvTuner
{
  Web100Widget web100widget;

  web100_var *LimRwin, *Rcvbuf;

  u_int32_t limrwin_val, limrwin_max, limrwin_min;
  u_int32_t rcvbuf_val, rcvbuf_max, rcvbuf_min;

  float limrwin_scale_val, rcvbuf_scale_val;

  GtkAdjustment *limrwin_adjustment, *rcvbuf_adjustment;
  GtkLabel *limrwin_label, *rcvbuf_label;
};

struct _RcvTunerClass
{ 
  Web100WidgetClass parent_class; 
};

#ifdef GTK2
GType       rcv_tuner_get_type    (void); 
#else
GtkType     rcv_tuner_get_type    (void);
#endif
GtkWidget*  rcv_tuner_new         (Web100Object *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RCV_TUNER_H__ */ 
