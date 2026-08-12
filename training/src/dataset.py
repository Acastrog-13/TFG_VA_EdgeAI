# Carga del dataset
import os
import tensorflow as tf


# Comprueba la existencia de distintas condiciones de luz (dia/noche) dentro de un semáforo
def get_condiciones(path_sem):

  contenido_sem = os.listdir(path_sem)
  condiciones = []
  if 'dia' in contenido_sem : condiciones.append ('dia')
  if 'noche' in contenido_sem : condiciones.append ('noche')
  if len(condiciones) == 0 : condiciones = [None]

  return condiciones

# Carga el archivo de la imagen
def cargar_imagen(path, label, image_size, reg=False):

  img = tf.io.read_file(path)
  img = tf.image.decode_jpeg(img, channels=3)
  img = tf.image.resize(img, image_size)
  img = tf.cast(img, tf.float32)

  if reg:
     label = tf.cast(label,tf.float32)

  return img, label

# Construye el dataset
def build_dataset(semaforos, path, labels, image_size, num_batches, train=False, reg=False):

  rutas, etiquetas = [], []

  for sem in semaforos:
      path_sem = os.path.join(path, sem)
      for cond in get_condiciones(path_sem):
          path_cond = os.path.join(path_sem, cond) if cond else path_sem
          for label in labels:
              path_label = os.path.join(path_cond, label)
              for img in os.listdir(path_label):
                  if img.lower().endswith('.jpg'):
                      rutas.append(os.path.join(path_label, img))
                      etiquetas.append(int(label))

  print(f'{len(rutas)} imágenes cargadas')

  ds = tf.data.Dataset.from_tensor_slices((rutas, etiquetas))

  if train:
      ds = ds.shuffle(buffer_size=len(rutas), seed=42, reshuffle_each_iteration=True)

  ds = ds.map(lambda path, label: cargar_imagen(path, label, image_size, reg),
        num_parallel_calls=tf.data.AUTOTUNE)
  ds = ds.batch(num_batches, drop_remainder=False)
  ds = ds.prefetch(tf.data.AUTOTUNE)
  return ds
