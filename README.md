# PiPatGen
Pattern generator, derived from Doz's https://andydoz.blogspot.com/2023/02/pipatgen-raspberry-pi-television.html

# SD IMAGE
find here a dumped image of the SD for pi zero : https://www.mediafire.com/file/7kaiuq4mylnqwzw/marcoexopatgen.img/file
user doz, password marcoexo

# CONNECTIONS
button connections : GPIO 18(PIN 12)  <--> GND (PIN 6) = HOME, GPIO 22 (PIN 15) <--> GND (PIN 6), GPIO 23 (PIN 16) <---> GND (PIN 6) 
(for a clear list of gpio pins : https://pinout.xyz/pinout/pin15_gpio22/) 

# Highlights on this pattern generator project

/boot/config.txt contains the operating mode (NTSC/PAL etc)
	#0 NTSC, 1 JAP, 2 PAL, 3 BRAZIL
	# uncomment for composite PAL
	sdtv_mode=2

/boot/retrogame.cfg contains the key definitions and port mapping for emulating usb keyboard (used for J and K keys next/prev for fbi image viewer)

/home/doz/.bashrc initializes everything, launching the retrogame program for emulating the J and K keys and the python program that launches the home sequence for FBI on GPIO 18 input (pin 12) (first image: 1G) launches the fbi program that displays the patterns present in the path /TestPatterns

these are the commands launched at startup (doz user autologin)

	 sudo /home/doz/keypress/retrogame &
  
	 sudo python /home/doz/Scripts/fbi_first_img.py &
  
	 fbi -a -noverbose /home/doz/TestPatterns/*.*


/home/doz/Scripts/fbi_first_img.py Python program that maps gpio 18 (pin 12) to the execution of the sendkeys for sending the 1G (home) sequence
/home/keypress/sendkeys.c is the program that sends a string to the system console by emulating a connected USB keyboard, it is used to send the 1G string
/home/keypress/retrogame.c is the program that allows you to map a series of GPIO ports to keys (adafruit) and is used to send the J and K (next/previous) commands to the fbi program that displays the images. The two keys are connected to GPIO 22 (ppin 15) and GPIO 23 (pin 16)
/home/doz/TestPatterns contains a set of useful images gathered from teh internet, replace/change with your preferred

-------------------ITA--------------------------------
# IMMAGINE DELLA SD

trova qui un'immagine scaricata della SD per pi zero: https://www.mediafire.com/file/7kaiuq4mylnqwzw/marcoexopatgen.img/file

# COLLEGAMENTI

connessioni pulsanti: GPIO 18(PIN 12) <--> GND (PIN 6) = HOME, GPIO 22 (PIN 15) <--> GND (PIN 6), GPIO 23 (PIN 16) <---> GND ( PIN6) 
(per un elenco chiaro dei pin gpio: https://pinout.xyz/pinout/pin15_gpio22/)

utente doz, password marcoexo

# Punti principali del progetto generatore di pattern

/boot/config.txt contiene il modo di funzionamento (NTSC/PAL ecc)

 \# 0 NTSC, 1 JAP, 2 PAL, 3 BRAZIL
 
 \# uncomment for composite PAL
 
sdtv_mode=2

/boot/retrogame.cfg contiene le definizioni dei tasti e la mappatura delle porte per emulare la tastiera USB (utilizzata per i tasti J e K successivo/precedente per il visualizzatore di immagini fbi)

/home/doz/.bashrc inizializza il tutto, lanciando il programma retrogame per l'emulazione dei tasti J e K e il pregramma python che lancia su input GPIO 18 (pin 12) la sequenza hoeme per FBI (prima immagine : 1G) lancia il programma fbi che visualizza i pattern presenti nel percorso /TestPatterns

questi i comandi lanciati all'avvio (autologin dell'utente doz)
	sudo /home/doz/keypress/retrogame &
	sudo python /home/doz/Scripts/fbi_first_img.py &
	fbi -a -noverbose /home/doz/TestPatterns/*.*


/home/doz/Scripts/fbi_first_img.py  Programma Python che mappa la gpio 18 (pin 12) alla esecuzione del sendkeys per l'invio della sequenza 1G (home)
/home/keypress/sendkeys.c è il programma che invia una stringa sulla console di sistema emulando una tastiera usb collegata, serve per invare la stringa 1G
/home/keypress/retrogame.c è il programma che consente di mappare una serie di porte GPIO a tasti (adafruit) ed è usato pper inviare i comandi J e K (successivo/precedente) al programma fbi che mostra le immagini. I due tasti sono collegati alla GPIO 22 (ppin 15) e GPIO 23 (pin 16)
/home/doz/TestPatterns contiene una serie di immagini utili raccolte da Internet, sostituisci/modifica con le tue preferite
