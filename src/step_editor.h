#ifndef _STEP_EDITOR_H_
#define _STEP_EDITOR_H_

G_BEGIN_DECLS

#define TYPE_STEP_EDITOR              (step_editor_get_type ())
#define STEP_EDITOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_STEP_EDITOR, StepEditor))
#define STEP_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_STEP_EDITOR, StepEditorClass))
#define IS_STEP_EDITOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_STEP_EDITOR))
#define IS_STEP_EDITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_STEP_EDITOR))
#define STEP_EDITOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_STEP_EDITOR, StepEditorClass))

typedef struct _StepEditorClass StepEditorClass;


struct _StepEditorClass
{
  GObjectClass parent_class;
};


struct _StepEditor
{
  GObject    parent;

  GtkWidget *widget;
  GtkWidget *container;
  GtkWidget *edit_widget;
  GtkWidget *scene_menu;

  GtkWidget *audio_file_entry;
  GtkWidget *file_selector;

  GList     *control;
  Step      *step;
};

GType        step_editor_get_type  (void) G_GNUC_CONST;
StepEditor  *step_editor_new       (Step        *step);
void         step_editor_set_title (StepEditor  *editor,
                                    const gchar *name);

G_END_DECLS

#endif /* _STEP_EDITOR_H_ */

