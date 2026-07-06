# Preprocesado de imágenes

import tensorflow as tf
import tf_keras as keras
from tf_keras import layers

class ImageCropY (layers.Layer) :

  def __init__(self, offset_y, **kwargs):
    super().__init__(**kwargs)
    self.offset_y = offset_y

  def call(self, img) :

    alto = tf.shape(img)[1]
    ancho = tf.shape(img)[2]
    lado = tf.minimum(alto, ancho)

    margen_y = (alto - lado) // 2

    crop_y = tf.cast(tf.cast(margen_y, tf.float32) * (1.0 + self.offset_y), tf.int32)
    crop_y = tf.clip_by_value(crop_y, 0, alto  - lado)

    return img[:, crop_y:crop_y + lado, :, :]

  def get_config(self):
    config = super().get_config()
    config.update({'offset_y': self.offset_y})
    return config

def preprocesado (image_size) :

  return keras.Sequential([
    keras.Input(shape=(image_size[0], image_size[1], 3), name='entrada_camara'),
    ImageCropY(0.5, name = 'crop_y'),
    layers.Resizing(224, 224, name = 'resize'),
    layers.Rescaling(1./127.5, offset=-1.0, name = 'normalizacion')
  ])
