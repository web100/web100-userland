#ifndef __SND_TUNER_H__
#define __SND_TUNER_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_SND_TUNER               (snd_tuner_get_type ())
#define SND_TUNER(obj)               (GTK_CHECK_CAST (obj, TYPE_SND_TUNER, SndTuner))
#define SND_TUNER_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_SND_TUNER, SndTunerClass))
#define IS_SND_TUNER(obj)            (GTK_CHECK_TYPE (obj, TYPE_SND_TUNER))
#define IS_SND_TUNER_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_SND_TUNER))

#ifdef GTK2
#define SND_TUNER_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_SND_TUNER, SndTunerClass))
#else
#define SND_TUNER_GET_CLASS(obj)     (SND_TUNER_CLASS (obj->klass))
#endif

typedef struct _SndTuner       SndTuner;
typedef struct _SndTunerClass  SndTunerClass;

struct _SndTuner
{
  Web100Widget web100widget;

  web100_var *LimCwnd, *Sndbuf;

  u_int32_t limcwnd_val, limcwnd_max, limcwnd_min;
  u_int32_t sndbuf_val, sndbuf_max, sndbuf_min;

  float limcwnd_scale_val, sndbuf_scale_val;

  GtkAdjustment *limcwnd_adjustment, *sndbuf_adjustment;
  GtkLabel *limcwnd_label, *sndbuf_label;
};

struct _SndTunerClass
{ 
  Web100WidgetClass parent_class; 
};

#ifdef GTK2
GType       snd_tuner_get_type    (void); 
#else
GtkType     snd_tuner_get_type    (void);
#endif
GtkWidget*  snd_tuner_new         (Web100Object *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SND_TUNER_H__ */ 
