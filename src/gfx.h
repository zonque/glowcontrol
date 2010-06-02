#ifndef _GFX_H_
#define _GFX_H_

GdkPixbuf *pixbuf_scene;
GdkPixbuf *pixbuf_channel;
GdkPixbuf *pixbuf_dimm;
GdkPixbuf *pixbuf_sleep;
GdkPixbuf *pixbuf_sync;
GdkPixbuf *pixbuf_audio;

GdkPixbuf *pixbuf_new (const gchar *filename);
gboolean   gfx_init (void);

#endif /* _GFX_H_ */
