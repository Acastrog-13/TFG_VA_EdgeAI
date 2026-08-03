import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import json

def curvas_entrenamiento(model_path, loss_name):

    with open(f'{model_path}/history.json', 'r') as f:
        hist = json.load(f)

    fig, axes = plt.subplots(1, 2, figsize=(12, 4))

    axes[0].plot(hist['loss'], label='Train loss')
    axes[0].plot(hist['val_loss'], label='Val loss')
    axes[0].set_title(f'Loss ({loss_name})')
    axes[0].set_xlabel('Epoch')
    axes[0].legend()

    axes[0].xaxis.set_major_locator(MaxNLocator(integer=True))
    axes[0].grid(True, linestyle='--', alpha=0.5)

    axes[1].plot(hist['mae_labels'], label='Train MAE')
    axes[1].plot(hist['val_mae_labels'], label='Val MAE')
    axes[1].set_title('MAE ordinal')
    axes[1].set_xlabel('Epoch')
    axes[1].legend()

    axes[1].xaxis.set_major_locator(MaxNLocator(integer=True))
    axes[1].grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    plt.savefig(f'{model_path}/curvas_entrenamiento.png', dpi=150)
    plt.show()
    