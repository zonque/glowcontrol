#ifndef _SCENE_EDITOR_H_
#define _SCENE_EDITOR_H_

G_BEGIN_DECLS

#define TYPE_SCENE_EDITOR              (scene_editor_get_type ())
#define SCENE_EDITOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SCENE_EDITOR, SceneEditor))
#define SCENE_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SCENE_EDITOR, SceneEditorClass))
#define IS_SCENE_EDITOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SCENE_EDITOR))
#define IS_SCENE_EDITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SCENE_EDITOR))
#define SCENE_EDITOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SCENE_EDITOR, SceneEditorClass))

typedef struct _SceneEditorClass SceneEditorClass;

struct _SceneEditorClass
{
  GObjectClass parent_class;
};


struct _SceneEditor
{
  GObject    parent;

  GtkWidget *widget;
  GtkWidget *name_entry;
  GList     *control;
  Scene     *scene;
};

GType        scene_editor_get_type  (void) G_GNUC_CONST;
SceneEditor *scene_editor_new       (Scene *scene);
void         scene_editor_set_title (SceneEditor *editor,
                                     const gchar *name);

G_END_DECLS

#endif /* _SCENE_EDITOR_H_ */

