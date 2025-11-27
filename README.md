# practica3-SETR
Información sobre la practica 3 de la asignatura Sistemas Empotrados y de Tiempo Real

## Mi Solución
He implementado en este sistema usando todo lo aprendido en clase.
- Para actualizar datos globales tal como sensores y el conteador de tiempo, he utilizado threads. Además de para eso también para implementar en parpadeo inicial del LED, con el fin de usar menos recursos (menos delays).
- Para evitar bloques inesperados, he usado watchdog (8s), reset() cuando es necesario para que funcione correctamente. Más de una vez watchdog ha reiniciado la placa cuando entraba en un bloquei sin saberlo.
- Para el funcionamiento de los botones, he usado las interrupciones hardware de los pines de arduino UNO. Con ellos detecto cuando el boton cambia de estado (CHANGE). Dentro de la función llamada tras este cambio diferencio entre FALLING y RISING dependiendo del estado actual del botón. En el primer caso guardo el tiempo (en ms) actual y en el segundo comparo ese tiempo con el nuevo, para ver si han pasado los 3 o 5 segundos requeridos. No es posible utilizar delays o utilizar comunicación, por lo que el código en esta función se basaba en guardar/comparar datos y cambiar flags.

## Video Explicativo
