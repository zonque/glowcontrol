#ifndef _PROJECT_H_
#define _PROJECT_H_

G_BEGIN_DECLS

#define TYPE_PROJECT              (project_get_type ())
#define PROJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PROJECT, Project))
#define PROJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PROJECT, ProjectClass))
#define IS_PROJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PROJECT))
#define IS_PROJECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PROJECT))
#define PROJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PROJECT, ProjectClass))

typedef struct _ProjectClass ProjectClass;

struct _ProjectClass
{
  GObjectClass parent_class;
};


struct _Project
{
  GObject  parent;

  gchar *filename;
  gboolean dirty;

  guint last_scene_id;

  GtkListStore *scene_store;
  GtkListStore *sequence_store;
  GtkListStore *channel_store;

  StepEditor   *step_editor;
};

GType     project_get_type      (void) G_GNUC_CONST;
Project  *project_new           (GtkListStore *scene_store,
                                 GtkListStore *sequence_store,
                                 GtkListStore *channel_store);
Project  *project_new_from_file (const gchar *filename,
                                 GtkListStore *scene_store,
                                 GtkListStore *sequence_store,
                                 GtkListStore *channel_store);

gboolean  project_write_to_file (Project *project,
                                 const gchar *filename);

/* Scenes */
void      project_add_scene     (Project *project,
                                 Scene   *scene);
void      project_set_scene_name (Project *project,
                                  Scene   *scene,
                                  gchar   *name);

void      project_open_scene_editor (Project *project,
                                     Scene   *scene);
Scene    *project_get_scene_by_id   (Project *project,
                                     guint    id);

/* Sequence */
void      project_add_step         (Project     *project,
                                    GtkTreeIter *sibling,
                                    Step        *step,
                                    GtkTreeIter *iter);
void      project_update_step      (Project     *project,
                                    Step        *step);
void      project_open_step_editor (Project     *project,
                                    Step        *step);

/* Channels */
gchar    *project_get_nth_channel_name (Project *project,
                                        gint     n);

G_END_DECLS

#endif /* _PROJECT_H_ */
