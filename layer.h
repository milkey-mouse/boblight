#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifndef PIXELS
/* RGB tuple representing a pixel. */
typedef struct rgbPixel
{
    uint8_t r, g, b;
} rgbPixel;

/* RGBA tuple representing a pixel with premultiplied alpha. */
typedef struct rgbaPixel
{
    uint8_t r, g, b, a;
} rgbaPixel;
#define PIXELS
#endif

/* Array of RGB pixels with length value. */
typedef struct rgbArray
{
    uint16_t length;
    rgbPixel *pixels;
} rgbArray;

/* Array of RGBA pixels with length value. */
typedef struct rgbaArray
{
    uint16_t length;
    rgbaPixel *pixels;
} rgbaArray;

/* Array of either RGB or RGBA pixels. */
typedef union pixArray {
    rgbPixel *rgb;
    rgbaPixel *rgba;
} pixArray;

/* Contains the pixel buffers and socket for a client,
   along with links to the previous and next layer to
   form a doubly-linked list. Layers are composited
   head -> tail (i.e. the tail layer is on top). */
typedef struct layer
{
    struct layer *prev;
    struct layer *next;
    rgbaArray channels[255];
    int sock;
} layer;

/* Initialize a layer for a new client and link it into
   the list at the end. */
layer *layer_init();

/* Remove a layer from the list without freeing the
   associated memory. */
void layer_unlink(layer *l);

/* Remove a layer from the list, freeing the pixel buffers
   and containing struct. */
void layer_destroy(layer *l);

/* Move the layer to the tail of the list, causing it
   to be composited in front of all others (up until
   another client connects or moves to the front). */
void layer_moveToFront(layer *l);

/* Move the layer to the head of the list, causing it
   to be composited behind all others (up until another
   client moves to the back, and excluding the static
   background layer). */
void layer_moveToBack(layer *l);

/* Move the layer towards the tail of the list, causing
   it to be composited one layer above where it was. */
void layer_moveUp(layer *l);

/* Move the layer towards the head of the list, causing
   it to be composited one layer below where it was. */
void layer_moveDown(layer *l);

/* Copy and premultiply alpha for length pixels from src
   to the specified channel in the given layer. If channel
   is 0, the pixels will be copied to every other channel. */
void layer_blit(layer *l, uint8_t channel, pixArray src, int length, bool alpha);

/* Iterate over the list of layers, compositing them with
   alpha blending into the static base layer. The layers
   are merged head -> tail, making the tail layer the top-
   most and the head layer the lowest. If c == 0, it will
   composite all channels. Otherwise, it will only do the
   channel with index c (1-255).*/
void layer_composite(uint8_t c);

/* Send the composited channel to the destination server
   in dest. layer_composite(c) must be called first for
   correct data. */
void layer_send(bamboozled_address *dest, uint8_t c);

/* Print the contents of composited channel c in a human-
   friendly way. This function is only used for debugging. */
void layer_repr(uint8_t c);

/* Bitfield that gets set to true when new pixel data is blitted
   to its channel index and gets set to false when the new data
   gets composited. */
uint32_t dirty[8];

/* pthreads conditional variable that fires when dirty == true. */
pthread_cond_t dirty_cv;

/* Mutex locking access to the dirty flag. */
pthread_mutex_t dirty_mutex;

/* Mutex locking the layers linked list. */
pthread_mutex_t layers_mutex;