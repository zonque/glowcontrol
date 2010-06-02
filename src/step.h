#ifndef _STEP_H_
#define _STEP_H_

G_BEGIN_DECLS

#define TYPE_STEP              (step_get_type ())
#define STEP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_STEP, Step))
#define STEP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_STEP, StepClass))
#define IS_STEP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_STEP))
#define IS_STEP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_STEP))
#define STEP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_STEP, StepClass))


typedef struct _StepClass StepClass;

typedef enum
  {
    STEP_TYPE_DIMM,
    STEP_TYPE_SLEEP,
    STEP_TYPE_SYNC,
    STEP_TYPE_AUDIO,
    STEP_TYPE_LAST
  }
StepType;

struct _StepClass
{
  GObjectClass parent_class;
};

struct _Step
{
  GObject       parent;

  StepType      type;
  gint          time;
  Scene        *scene;
  gchar        *filename;

  Project      *project;

  gint          progress;
};


GType     step_get_type  (void) G_GNUC_CONST;
Step     *step_new       (Project   *project);
gboolean  step_tick      (Step      *step);
void      step_dump      (Step      *step,
                          XMLWriter *writer);

G_END_DECLS

#endif /* _STEP_H_ */

