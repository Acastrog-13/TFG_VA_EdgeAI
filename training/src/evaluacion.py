# Evaluación del modelo

import json

import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
import seaborn as sns
import coral_ordinal as coral
from scipy.optimize import minimize

from sklearn.metrics import (
    cohen_kappa_score,
    classification_report,
    confusion_matrix,
    mean_absolute_error,
    f1_score
)

def print_confusion_matrix (y_true, y_pred_labels, labels, path):

    nombres_clases = list(labels.values())

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
    plt.savefig(f"{path}/confusion_matrix.png", dpi=150)
    plt.show()

    return

def calculo_report (y_true, y_pred_labels, labels, path):

    nombres_clases = list(labels.values())

    # Classification report
    report = classification_report(
        y_true, 
        y_pred_labels,
        target_names=nombres_clases
    )
    print(f'\n  Classification report:')
    print(report)

    with open(f'{path}/classification_report.txt', 'w') as f:
        f.write(report)

    return report


def calculo_metricas(y_true, y_pred_labels, labels, path):

    # Cálculo de métricas
    mae = np.mean(np.abs(y_true - y_pred_labels))
    qwk = cohen_kappa_score(y_true, y_pred_labels, weights="quadratic")
    acc = np.mean(y_true == y_pred_labels)
    acc_1off = np.mean(np.abs(y_true - y_pred_labels) <= 1)
    sesgo = np.mean(y_pred_labels - y_true)

    print("MÉTRICAS:")
    print(f"Accuracy:       {acc:.4f}")
    print(f"1-off Accuracy: {acc_1off:.4f}")
    print(f"MAE:            {mae:.4f}")
    print(f"QWK:            {qwk:.4f}")
    print(f"Sesgo:          {sesgo:.4f}")
    print()

    # Classification report
    report = calculo_report(y_true, y_pred_labels, labels, path)

    # Matriz de confusión
    print_confusion_matrix(y_true, y_pred_labels, labels, path)

    # Guardar métricas
    metricas = {
        "accuracy": float(acc),
        "acc_1off": float(acc_1off),
        "mae": float(mae),
        "qwk": float(qwk),
        "sesgo": float(sesgo),
        "classification_report": report
    }

    with open(f"{path}/metricas.json", "w") as f:
        json.dump(metricas, f, indent=2)

    print("Resultados guardados en:", path)

    return metricas
    

def evaluar_modelo(model, test_ds, labels, model_path):

    y_pred_logits = model.predict(test_ds)
    y_pred_probs = tf.sigmoid(y_pred_logits).numpy()
    y_pred_labels = (y_pred_probs > 0.5).sum(axis=1)


    # Etiquetas reales
    y_true = np.concatenate([y for _, y in test_ds], axis=0)

    # Cálculo de métricas
    return calculo_metricas(y_true, y_pred_labels, labels, model_path)


def evaluar_modelo_reg(model, test_ds, labels, model_path, limits=[0.5, 1.5, 2.5]):

    y_pred_cont = model.predict(test_ds, verbose=0).flatten()

    # Etiquetas reales
    y_true_cont = np.concatenate([y for _, y in test_ds], axis=0).flatten()
    y_true = y_true_cont.astype(int)

    # Discretización
    y_pred_labels = np.zeros(len(y_pred_cont), dtype=int)
    y_pred_labels[y_pred_cont >= limits[0]] = 1
    y_pred_labels[y_pred_cont >= limits[1]] = 2
    y_pred_labels[y_pred_cont >= limits[2]] = 3

    # Cálculo de métricas
    mae = np.mean(np.abs(y_true - y_pred_labels))
    mae_cont = float(mean_absolute_error(y_true_cont, y_pred_cont))
    qwk = cohen_kappa_score(y_true, y_pred_labels, weights="quadratic")
    acc = np.mean(y_true == y_pred_labels)
    acc_1off = np.mean(np.abs(y_true - y_pred_labels) <= 1)
    sesgo = float(np.mean(y_pred_labels - y_true))
    sesgo_continuo = float(np.mean(y_pred_cont - y_true_cont))

    print("MÉTRICAS:")
    print(f"Accuracy:       {acc:.4f}")
    print(f"1-off Accuracy: {acc_1off:.4f}")
    print(f"MAE Discreto:   {mae:.4f}")
    print(f"MAE Continuo:   {mae_cont:.4f}")
    print(f"QWK:            {qwk:.4f}")
    print(f"Sesgo Discreto: {sesgo:.4f}")
    print(f'Sesgo Continuo: {sesgo_continuo:.4f}')
    print(f"Umbrales:       {limits}")
    print()

    # Classification report
    report = calculo_report(y_true, y_pred_labels, labels, model_path)

    # Matriz de confusión
    print_confusion_matrix(y_true, y_pred_labels, labels, model_path)

    # Guardar métricas
    metricas = {
        "accuracy": float(acc),
        "acc_1off": float(acc_1off),
        "mae_discreto": float(mae),
        "mae_continuo": float(mae_cont),
        "qwk": float(qwk),
        "sesgo_discreto": float(sesgo),
        "sesgo_continuo": float(sesgo_continuo),
        "umbrales": limits,
        "classification_report": report
    }

    with open(f"{model_path}/metricas.json", "w") as f:
        json.dump(metricas, f, indent=2)

    print("Resultados guardados en:", model_path)

    return metricas


def optimizar_umbrales (dataset, model, limits_ini, intervals, min_with):

    y_pred_cont = model.predict(dataset, verbose=0).flatten()
    
    y_true_cont = np.concatenate([y for _, y in dataset], axis=0).flatten()
    y_true = y_true_cont.astype(int)

    def func(limits):

        if not (intervals [0][0] <= limits[0] <= intervals[0][1] and
                intervals [1][0] <= limits[1] <= intervals[1][1] and
                intervals [2][0] <= limits[2] <= intervals[2][1]) :
            return 1e6

        if (limits[1] - limits[0] < min_with or
            limits[2] - limits[1] < min_with):
            return 1e6
        
        y_pred_disc = np.digitize(y_pred_cont, limits)

        score = f1_score(y_true, y_pred_disc, average='macro', zero_division=0)

        return -score

    res = minimize (
        func,
        limits_ini,
        method='Nelder-Mead',
        options={'maxiter':2000}
    )

    return np.sort(res.x)
