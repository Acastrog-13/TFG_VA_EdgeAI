# Evaluación del modelo

import json

import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
import seaborn as sns

from sklearn.metrics import (
    cohen_kappa_score,
    classification_report,
    confusion_matrix,
)


def evaluar_modelo(model, test_ds, labels, model_path):

    y_pred_logits = model.predict(test_ds)
    y_pred_probs = tf.sigmoid(y_pred_logits).numpy()
    y_pred_labels = (y_pred_probs > 0.5).sum(axis=1)

    # Etiquetas reales
    y_true = np.concatenate([y for _, y in test_ds], axis=0)

    # Cálculo de métricas
    mae = np.mean(np.abs(y_true - y_pred_labels))
    qwk = cohen_kappa_score(y_true, y_pred_labels, weights="quadratic")
    acc = np.mean(y_true == y_pred_labels)
    acc_1off = np.mean(np.abs(y_true - y_pred_labels) <= 1)

    print("MÉTRICAS:")
    print(f"Accuracy:       {acc:.4f}")
    print(f"1-off Accuracy: {acc_1off:.4f}")
    print(f"MAE:            {mae:.4f}")
    print(f"QWK:            {qwk:.4f}")
    print()

    nombres_clases = list(labels.values())

    print(
        classification_report(
            y_true,
            y_pred_labels,
            target_names=nombres_clases
        )
    )

    # Matriz de confusión
    cm = confusion_matrix(y_true, y_pred_labels)

    plt.figure(figsize=(6, 5))
    sns.heatmap(
        cm,
        annot=True,
        fmt="d",
        cmap="Blues",
        xticklabels=nombres_clases,
        yticklabels=nombres_clases,
    )

    plt.xlabel("Predicción")
    plt.ylabel("Real")
    plt.title("Matriz de confusión")
    plt.tight_layout()
    plt.savefig(f"{model_path}/confusion_matrix.png", dpi=150)
    plt.show()

    # Guardar métricas
    metricas = {
        "accuracy": float(acc),
        "acc_1off": float(acc_1off),
        "mae": float(mae),
        "qwk": float(qwk),
    }

    with open(f"{model_path}/metricas.json", "w") as f:
        json.dump(metricas, f, indent=2)

    print("Métricas guardadas en:", model_path)

    return metricas