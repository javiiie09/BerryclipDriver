Se entrega:

	Código en C del driver para la placa Berryclip
	Makefile (Proporcionado por el profesor en clase)
	Memoria descriptiva del código

Para compilar haremos uso del Makefile con el comando

	make MODULE=berryclip_driver compile

Para iniciar el módulo usaremos el siguiente comando, podremos modificar
los milisegundos de debounce (No es obligatorio el parámetro):

	sudo insmod berryclip_driver.ko  debounce=200

Para eliminar el módulo:

	sudo rmmod berryclip_driver

Para escribir en /dev/leds desde terminal se puede puede hacer uso del
comando:

	echo -ne "\x00" > /dev/leds
