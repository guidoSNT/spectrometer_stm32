#import "@preview/fletcher:0.5.1" as fletcher: diagram,node,edge,shapes
#set par(justify: true)
#set text(lang: "sp",size: 11pt)
#set page(
  header: [_UNLP - Ingenieria Electronica_ #h(1fr) _Potente Guido - 2024_] + line(length: 100%),
  numbering: "-1-",
)
#let codeText(body, fill: rgb("#138bbc"), weight: "bold", size: 11pt)={
  text(
    fill: fill, font: "DM Mono", size: size, weight: weight,
  )[#body]
}



#align(center)[
#text(size: 17pt,weight: "bold",style: "italic")[*Problema especial 2*]
#text(size: 11pt,style: "italic" )[\ UNLP - Facultad de Ingenieria \ Sistemas embebidos - E1504 \ Potente Guido - 73230/5]
]

 = Planteo de la resolucion
 == Consigna base
En primer lugar se busco un diseño del sistema para cumplir con las consignas de modo que se cumpla:

 + Captura del #codeText("adc") a $1 k H z$ mostrando en pantalla la #codeText("FFT") cada 256 muestras.
 + Uso de un cursor que permita ver el valor actual de tension y frecuencia en ese punto.
 + Creacion de una #codeText("PWM") con el #codeText("dma") para pasarla por un pasabajos y obtener una sinusoidal a muestrear.
 + Encoder con el que mover el cursor.
 + Botones para seleccionar cosas en el menu y cambiar entre pantallas.
 + Una pantalla de configuracion, una para la #codeText("FFT") y una para ver los valores actuales.
 + Uso de 3 tareas y algun metodo de sincronizacion.

 Ademas, en la pantalla de configuracion se deben tener los siguientes botones:
 - Cambio de amplitud de la #codeText("PWM").
 - Cambio de la frecuencia de la #codeText("PWM").
 - Impresion por #codeText("UART") de los valores de los valores actuales medidos.
 - Cambio entre el encoder como cursor o cambiando las variables de la #codeText("PWM").

 Con esto en mente se procedio con un esquematico basico de las interfaces esperadas.

 == Idea basica del esquema
 Para tener un esquema de lo buscado con las pantallas se dibujo en papel algunos ejemplos hasta llegar al resultado deseado. Las siguientes figuras muestras los esquemas finales con los que se ideo el resto.
#figure(image("images/config_diagrama.jpeg",width: 50%),caption: text("Diagrama basico de la configuracion",size: 10pt,style: "italic"))

#figure(image("images/fft_diagrama.jpeg",width: 50%),caption: text("Diagrama basico de la fft en modo cursor y con la pwm",size: 10pt,style: "italic"))

 Como se puede ver se removio la pantalla de visualizacion de los valores actuales porque la #codeText("FFT") con el cursor ya tienen esta informacion. Ademas, la funcionalidad del puntero en la configuracion lo que hace es seleccionar la opcion y con el encoder modificarla de modo que aumente o disminuya lo que se esta haciendo.

 == Toma de datos

 En cuanto a los datos que deben ser tomados se decidio usar interrupciones para todos con la finalidad de poder ahorrar la creacion de una tarea que tenga que estar constantemente sampleando todas las entradas. Para esto se tendran que incorporar por interrupcion estas cosas:
 - #codeText("adc") con un timer cada $1 m s$.
 - #codeText("PWM") con el #codeText("DMA") para no tener que estar manualmente actualizando.
 - Encoder con interrupcion por #codeText("GPIO").
 - Botones con timer para usar estrategia antirebote.

 Por lo tanto solo seran necesarias dos tareas para hacer todo el proceso.

 = Implementacion de funciones
 
 == PWM
 Para poder hacer esta onda especifica se tomo lo dado en clase generando los valores del duty cycle para cada ciclo de la #codeText("PWM") y despues enviar esto con el #codeText("DMA"). Esto permite precision en la onda final, pero genera problemas a hora de hacer frecuencias muy bajas respecto a la #codeText("PWM") pues aumenta la cantidad de puntos a almacenar. Por lo tanto, con esto se planteo usar una funcion para setear una frecuencia y amplitud.

 == Encoder
 El encoder tiene la caracteristica de que es ruidoso y muchas veces se puede volver dificil detectar un cambio correcto sin una estrategia antirebote. Entonces, se tomo la maquina de estados dada en clase que setee la posicion respecto a un offset.

 #align(center)[

#diagram(
  node-stroke: .1em,
  node-fill: gradient.radial(blue.lighten(80%), blue, center: (30%, 20%), radius: 80%),
  spacing: 4em,
  edge((-1,0), "r", "-|>", `Inicio`, label-pos: 0, label-side: center),
  node((0,0), `idle`, radius: 2em),
  edge(`A`, "-|>",bend:10deg,label-side:center),
  node((1,0), `A1`, radius: 2em),
  edge(`AB`, "-|>",label-side:center),
  node((2,0), `A2`, radius: 2em),
  edge(`B`,"-|>",label-side:center),
  node((3,0),`A3`,radius: 2em),
  edge((0,0), (0,0), `none`, "-|>", bend: 130deg,label-side:center),
  edge((2,0), (0,0), `none`, "-|>", bend: 30deg,label-side:center),
  edge((1,0),(0,0), `none`,"-|>",bend:10deg,label-side:center),
  edge((3,0),(0,0),`none/{pos++}`,"-|>",bend:-30deg,label-side:center),
  edge((2,0),(1,0),`A`,"-|>",bend:-10deg),
  edge((3,0),(2,0),`AB`,"-|>",bend:-10deg),
  node((0,1),`B2`,radius:2em),
  node((1,1),`B1`,radius:2em),
  node((-1,1),`B3`,radius:2em),
  edge((0,0),(1,1),`B`,"-|>",bend:-10deg,label-side:center),
  edge((0,0),(1,1),`none`,"<|-",bend:10deg,label-side:center),
  edge((0,1),(0,0),`none`,"-|>",label-side:center),
  edge((1,1),(0,1),`AB`,"-|>",label-side:center),
  edge((0,1),(-1,1),`A`,"-|>",label-side:center),
  edge((-1,1),(0,0),"-|>",`none/{pos--}`,label-side:center,label-angle:auto),
  edge((0,1),(1,1),`B`,"-|>",bend:-10deg),
  edge((-1,1),(0,1),`AB`,"-|>",bend:-10deg)
)
]
Como se puede ver esta maquina avanza en los estados hasta llegar a comprobar que es correcta realmente se pudo hacer un paso del encoder. Cabe aclarar que los eventos son dependiendo de que pin este en bajo, como por ejemplo el #codeText("AB") representa cuando ambos pines estan activos.

== Botones
En este caso tambien se implemento una estrategia antirebote por lo que necesariamente se debe hacer un muestreo constante a estos. La maquina de estados de estos tambien se dio en clase por lo que nuevamente se hace lo mismo.
 #align(center)[

#diagram(
  node-stroke: .1em,
  node-fill: gradient.radial(blue.lighten(80%), red, center: (30%, 20%), radius: 80%),
  spacing: 4em,
  edge((-1.,0),"<|-",`Inicio`,label-pos:0.7),
  node((0,0),`Off`,radius:2em),
  node((2,0),`Pre-off`,radius:2em),
  node((0,2),`Pre-on`,radius:2em),
  node((2,2),`On`,radius:2em),
  edge((0,0),(0,2),`set\{cnt=0}`,"-|>",label-side:center,label-angle:left,bend:-10deg),
  edge((0,2),(0,0),`N-timeout`,"-|>",label-side:center,label-angle:right,bend:-10deg),
  edge((0,2),(2,2),`Y-timeout`,"-|>",label-side:center,bend:-10deg),
  edge((2,2),(2,0),`reset/{cnt=0}`,"-|>",label-angle:right,label-side:center,bend:-10deg),
  edge((2,0),(2,2),`N-timeout`,"-|>",label-angle:left,label-side:center,bend:-10deg),
  edge((2,0),(0,0),`Y-timeout`,"-|>",label-side:center,bend:-10deg),
  edge((0,0),(0,0),`reset/{cnt=0}`,label-side:center,"-|>",bend:130deg),
  edge((2,0),(2,0),`reset/{cnt++} or set/{cnt--}`,label-side:center,"-|>",bend:130deg),
  edge((0,2),(0,2),`reset/{cnt--} or set/{cnt++}`,label-side:center,"-|>",bend:-130deg),
  edge((2,2),(2,2),`set/{cnt=0}`,label-side:center,"-|>",bend:-130deg),
)
]
Por lo tanto con los botones usando un timer y el encoder con las interrupciones por #codeText("GPIO") se puede no usar una tarea para leer variables. Falta el #codeText("adc") pero este simplemente lee en el valor y lo guarda en un arreglo con la interrupcion de un timer.

== UART
Para pasar los datos se puede hacer bloqueante, pero es conveniente que no tarde mucho por lo que se impuse la maxima frecuencia asincronica que soporta el controlador de #codeText("UART") y la _Blue Pill_ ($921600 space b p s$). Ademas cree un arreglo con las strings para los 128 valores a pasar de modo que cada vez que se escribe unicamente se necesita escribir este los valores y pasarlo.



= Interrupciones y tareas

== Interrupciones
Como ya se menciono, se tienen las siguientes interrupciones configuradas:

- Timer para el #codeText("adc") con prioridad #codeText(fill: red,"5") que es la mayor posible con la configuracion de #codeText("FreeRTOS") actual.
- #codeText("GPIO") interrupt en bajada y subida para el encoder con prioridad #codeText(fill: red,"6") por lo que es critico como el #codeText("adc") y no es tan importante la temporizacion pues simplemente se debe usar para detectecar si se movio o no.
- Timer para los botones integrado con el mismo del #codeText("SysTick") que tiene la menor prioridad (#codeText(fill:red,15)) posible. Nuevamente la temporizacion no es importante pues lo unico que puede cambiar es que se modifique el tiempo del contador anti-rebote, pero no es algo grave a su funcionamiento.

== Tareas
Como no se debe usar la tarea del monitoreo solamente nos queda una que actualiza pantalla y otra que procesa los datos para la #codeText("FFT"). Teniendo en cuenta que el proceso de la pantalla suele tomar $30m s$ mientras que el procesamiento solo se hace en una pantalla y cada $256m s$ es logico asignar mayor prioridad a esta pues es la que menor tiempo consume. Mas aun, la actualizacion de pantalla puede retrasarse cerca de $70 m s$ pues eso le daria cerca de $100 m s$ de respuesta.

= Solucion
== Sincronizacion

Para sincronizar las tareas se usaron dos mutex ya que uno habilita la tarea de procesamiento y el otro es para bloquear cuando se esta procesando. Esto es porque mientras no este en la pantalla de la #codeText("FFT") no necesito calcular nada, pero cuando sea necesario es importante no actualizar la pantalla hasta que esten los datos porque el buffer de datos es uno solo lo que podria ocasionar problemas de lectura.

Por lo tanto los mutex son:
- #codeText("adcSave") $-->$ Bloquea el procesamiento hasta estar en la pantalla de la #codeText("FFT").
- #codeText("fftRead") $-->$ Bloquea la tarea de pantallas hasta que se termine el calculo de la #codeText("FFT").

== Modo de operacion ideado
Teniendo en cuenta los puntos marcados se decidio que finalmente la idea es:
- #codeText("Pantalla configuracion",fill:orange)
 - 4 opciones que se recorren con un cursor que se mueve a partir del encoder.
 - Si el encoder se apreta una vez selecciona el item y no se mueve.
 - Los valores de tension/frecuencia  y la funcion del encoder se modifican girando el encoder.
 - Los valores finales se ejecutan unicamente cuando se aprete nuevamente el encoder.
- #codeText("Pantalla FFT",fill:orange)
 - Con un segundo boton se cambia entre esta y la cofiguracion.
 - En caso de ser con cursor el movimiento del encoder mueve de a 1 Hz.
 - El muestreo solo para cuando se calcula y actualiza la pantalla.
 - En caso de ser con #codeText("PWM") el encoder mueve la frecuencia o amplitud.
== Maquina de estados
Puesto que la maquina de estados se volvio muy complicada de implementar completa en una sola se plantearon dos maquinas iguales pero que ejecutan distintas acciones y donde una esta dentro de la otra. Esto refiere a que hay una maquina para los displays y dependiendo del estado y evento de esta, se ejecuta otra maquina identica pero de la pantalla de configuracion.


 #align(center)[
#diagram(
  node-stroke: .1em,
  node-fill: gradient.radial(yellow.lighten(80%), green, center: (30%, 20%), radius: 80%),
  spacing: 4em,
  edge((-1.,0),"<|-",`Inicio`,label-pos:0.7),
  node((0,0),`Config`,radius:3em),
  node((3,0),`Config-pre-off`,radius:3em),
  node((0,3),`Fft-pre-off`,radius:3em),
  node((3,3),`fft`,radius:3em),
  edge((0,0),(0,3),`on/{Config sfm}`,"-|>",label-side:center,bend:-10deg),
  edge((0,3),(3,3),`off/{FFT & Datos}`,"-|>",label-side:center,bend:-10deg),
  edge((3,3),(3,0),`on/{Datos}`,"-|>",label-side:center,bend:-10deg),
  edge((3,0),(0,0),`off/{Config sfm}`,"-|>",label-side:center,bend:-10deg),
  edge((0,0),(0,0),[`off_update/{Config sfm}` \ `off_stay/{NULL}`],"-|>",bend:130deg),
  edge((3,0),(3,0),`on/{NULL}`,"-|>",bend:130deg),
  edge((0,3),(0,3),`on/{NULL}`,"-|>",bend:-130deg),
  edge((3,3),(3,3),[`off_update/{FFT & mutex}` \ `off_stay/{mutex}`],"-|>",bend:-130deg),
)
]

Los eventos estan determinados por las siguientes condiciones:

 - #codeText("off_stay") $-->$ No hay actualizacion de botones, encoder o pedido de #codeText("FFT") y el boton 2 esta apagado.
 - #codeText("off_update") $-->$ Hay alguna actualizacion y el boton 2 esta apagado.
 - #codeText("on_stay") $-->$ No hay actualizacion de botones, encoder o pedido de #codeText("FFT") y el boton 2 esta prendido.
 - #codeText("off_update") $-->$ Hay alguna actualizacion y el boton 2 esta prendido.

 Cabe aclarar que los #codeText("on") o los #codeText("off") significan que sea con o sin actualizacion se ejecuta eso.

 Por otro lado las acciones son:

 - #codeText("update") $-->$ En cualquier caso que haya un evento con _update_ se actualiza la pantalla.
 - #codeText("Config sfm") $-->$ Llamado a la maquina de la pantalla de configuracion.
 - #codeText("FFT")  $-->$ Grafica la #codeText("FFT").
 - #codeText("Datos") $-->$ Libera o toma el mutex que permite a la tarea de procesamiento funcionar.
 - #codeText("mutex") $-->$ Intenta tomar el mutex que calcula la #codeText("FFT") de modo que espere en caso de no estar diponible.

 Aunque la maquina de estados de la pantalla de configuracion es identica, toma acciones distintas en cada caso, pero para resumir su uso lo que se hace es tener un #codeText("struct") accesible solo a la libreria que tiene banderas que pueden llamarse con funciones #codeText("getter") y ejecutar desde el #codeText("main").

 La razon de esta ejecucion fuera de la maquina es porque los botonoes se guardan en una cola que se debe leer en la tarea y pasar todos los valores a la maquina antes de obtener el estado actual. Esto significa que no tiene sentido actualizar la pantalla con un valor de la cola que instantaneamente se va a modificar.

 En definitiva la maquina de la pantalla #codeText("config") es:

 
 #align(center)[
#diagram(
  node-stroke: .1em,
  node-fill: gradient.radial(white.lighten(80%), orange, center: (30%, 20%), radius: 80%),
  spacing: 4em,
  edge((-1.,0),"<|-",`Inicio`,label-pos:0.7),
  node((0,0),`Not_off`,radius:3em),
  node((3,0),`Sel-on`,radius:3em),
  node((0,3),`Not-on`,radius:3em),
  node((3,3),`Sel-off`,radius:3em),
  edge((0,0),(0,3),`pressed/{fill=1}`,"-|>",label-side:center,bend:-10deg),
  edge((0,3),(3,3),`not/{fill & set_old}`,"-|>",label-side:center,bend:-10deg),
  edge((3,3),(3,0),`pressed/{NULL}`,"-|>",label-side:center,bend:-10deg),
  edge((3,0),(0,0),`not/{Set values}`,"-|>",label-side:center,bend:-10deg),
  edge((0,0),(0,0),[`not/{pwm & send=0}`],"-|>",bend:130deg),
  edge((3,0),(3,0),`pressed/{NULL}`,"-|>",bend:130deg),
  edge((0,3),(0,3),`pressed/{NULL}`,"-|>",bend:-130deg),
  edge((3,3),(3,3),`not/{Set option}`,"-|>",bend:-130deg),
)
]


Los eventos estan determinados por las siguientes condiciones:

 - #codeText("not") $-->$ Boton del encoder apagado.
 - #codeText("on") $-->$ Boton del encoder prendido.

 Por otro lado las acciones son:

 - #codeText("pwm") $-->$ Indica que se actualiza bandera para actualizar pwm.
 - #codeText("fill") $-->$ Indica si la flecha del menu se debe rellenar o no.
 - #codeText("send")  $-->$ Especifica si mandar o no por uart.
 - #codeText("set_old") $-->$ Guarda el valor actual de la configuracion de la pantalla.
 - #codeText("Set option") $-->$ Selecciona en base a la posicion del encoder una de las 4 opciones en pantalla y actualiza sus variables en caso de ser necesario.
 - #codeText("Set values") $-->$ Activa flag para actualizar #codeText("PWM"), para cambiar el relleno de la flecha y dependiendo si se habia seteado para mandar por uart envia.

 Por otro lado, siempre que entra a #codeText("off") guarda el ultimo valor del encoder y en #codeText("on") guarda el valor de #codeText("set_old") en el actual. Esto es para evitar problemas en caso de que se actualize la variable por alguna otra razon.

Finalmente uno llama desde el #codeText("main") a una funcion de la libreria que en base a estas banderas dibuja la pantalla. Sin embargo todo lo que pueda tardar varios ms se pasa con in #codeText("getter") al main y de ahi se actualiza (lo unico que no cumple con esto es la #codeText("UART")).

= Uso de memoria

Observando las tareas durante la compilacion se obtuvieron los siguientes resultados:
#figure(
image("images/tareas.png",width: 70%),caption: "Depuracion en la compilacion"
)

Como se puede ver, las tareas #codeText("entryDisplay") que procesa las pantallas consumira aproximadamente 676 bytes o 169 palabras. Por otro lado #codeText("entryDatos") consume 248 bytes o 71 palabras.

Por otro lado, usando el debugger para ver los stacks en distintos puntos de cada tarea se pudo obtener estos valores maximos:
#figure(image("images/dis_stack.png"),caption: "Maximo consumo encontrado en la tarea de los displays")
#figure(image("images/fft_stack.png"),caption: "Maximo consumo encontrado en la tarea que procesa los datos")

Traduciendo a bytes/palabras se tiene que:

- #codeText("display") $-->$ 464 bytes / 116 palabras (menor a lo visto en la compilacion).
- #codeText("fft") $-->$ 924 bytes / 231 palabras (mayor a lo visto en compilacion).

Tomando el peor caso de cada uno y agregando un extra de 20% se tiene que #codeText("display") tendra 203 palabras y #codeText("fft") tendra 277. Ademas, esto nos determina en gran parte cuanto stack necesitamos para el #codeText("FreeRTOS") de modo que observando el heap usado:
#figure(image("images/heapo.png",width: 50%))
Esta utilizando 2448 bytes por lo que nuevamente poniendo un margen del 20% se puede dejar en 2937 bytes de modo que finalmente el uso de RAM y Flash quede:
#image("Screenshot from 2024-08-02 03-51-51.png")

= Temporizacion de las tareas

Para observar el factor de uso se dispuso de distintas situaciones:

+ #codeText("Pantalla configuracion en idle")
  - En este caso se deja la pantalla de configuracion quieta sin tocar botones o el encoder de modo que se ejecuta solo la tarea de los diplays con esta tasa:
  


#figure(image("images/DS1Z_QuickPrint1.png",width: 70%),caption: "Hook de la tarea de display sin interactuar")
Como se puede se llama cada 50 ms y como no ocurren nada sale en $26 mu s$, pero tambien hay que tener en cuenta el llamado a la interrupcion del boton cada $1 m s$, lo que tarda $7.6 mu S$.

$
(26 mu s + 50 times 7.6 u s )/(50 m s + 26 mu s) times 100% approx 0.81 %
$

2. #codeText("Moviendo constantemente")
 - Si fuese posible mover constantemente el encoder o algun boton se obtendria la mayor tasa de uso en la pantalla de configuracion. Esto es debido a que en todos los llamados a la tarea se tendra que actualizar la pantalla, que tarda cerca de $30 m s$.
 #figure(image("images/DS1Z_QuickPrint3.png",width: 70%),caption: "Hook de la tarea display cuando se debe actualizar")

 Con esto ya se puede calcular teniendo en cuenta nuevamente la interrupcion:

 $
 ( 32.6 m s + 50 times 7.6 mu s )/(50 m s + 32.6 m s) times 100% approx 39.92%
 $

 3. #codeText("Mostrando FFT")
  - En caso de que se muestre la #codeText("FFT") el uso aumenta pues la otra tambien esta funcionando, como tambien la interrupcion del adc. Esto implica que es mas simple calcular en base a lo observando en la idle y midiendo las interrupciones.

#figure(image("images/DS1Z_QuickPrint4.png"),caption: "Tarea Idle durante la pantalla de la FFT")

Entonces los datos no conocidos que se midieron son el calculo de la #codeText("FFT") (22ms), el adc que tarda $21.2 mu s$ y el tiempo que esta en alto el idle ($223 m s$).

$
(22 m s + 32.6 m s + 21.2 u s times 256 + 7.6 mu s times 277)/(223+22+32.6) times 100% approx 22.38%
$

Claramente si uno mueve el cursor aumentara el uso pero solo momentaneamente porque no es posible mover constantemente el encoder.
