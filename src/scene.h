#ifndef _SCENE_H_
#define _SCENE_H_

G_BEGIN_DECLS

#define TYPE_SCENE              (scene_get_type ())
#define SCENE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SCENE, Scene))
#define SCENE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SCENE, SceneClass))
#define IS_SCENE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SCENE))
#define IS_SCENE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SCENE))
#define SCENE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SCENE, SceneClass))


typedef struct _SceneClass SceneClass;

struct _SceneClass
{
  GObjectClass parent_class;
};

struct _Scene
{
  GObject       parent;

  guint         id;

  gchar        *name;
  gint          channel[N_CHANNELS];
  Project      *project;
  SceneEditor  *editor;
};


GType  scene_get_type        (void) G_GNUC_CONST;
Scene *scene_new             (Project *project);
Scene *scene_duplicate       (Scene *scene);
void   scene_set_value       (Scene *scene,
                              gint n,        
                              gint val);
void   scene_send_all_values (Scene *scene);
void   scene_dump            (Scene *scene,
                              XMLWriter *writer);

G_END_DECLS

#endif /* _SCENE_H_ */

