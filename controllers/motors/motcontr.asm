; PLANETARIO DI MARGHERA
; CONTROLLO MOTORI
; RELEASE 1.0
; RV271199-RV071299
; Introduzione zero encoder: RV270200
; 4800 baud anziche' 9600, allungato buffer rx da 32 a 48 ch: RV070400
;
; INCLUDE:
; FUNZIONI DI LIBRERIA E2PROM (EX E2LIB.ASM)
;
;
; 1) ROUTINE DI ACCESSO AL CONTROLLORE MOUSE PS/2
;    PER LA LETTURA ENCODER MOTORI
; 2) ROUTINE DI SCRITTURA DAC
;
; Initialize: sequenza di inizializzazione per tutte le funzioni di libreria
; SendMouse : invia byte al mouse
; RecvMouse : legge byte dal mouse
; MouseRead : legge messaggio posizione da mouse e accumula su 2x24 bit
; serXxx    : interfaccia seriale
; AddXxx    : somme su 24 bit
; wrSpi     : scrive sui 2 DAC tramite SPI
;
; INTERFACCIA MOUSE:
; DATA:  PC0 e PC2 in parallelo
; CLOCK: PC1 e PC3 in parallelo
; RBTN:  PB2 attraverso diodo (K->bottone)
; TX: cambia stato DATA dopo CLOCK H->L come verificato
;     su PC Armada
;
; INTERFACCIA DAC MAX549 (comando PWM motore)
; \CS:  PD5 (\SS)   chip select ATTIVO BASSO
; DIN:  PD3 (MOSI)  linea dati
; SCLK: PD4 (SCK)   clock ATTIVO ALTO
;
; INTERFACCIA MOTORE (verso di rotazione)
; SIGN: PB0, PB1
; ~SIGN: PB3, PB4	(per comando in modo "enable switching") NON USATO!
;
;
; COSTANTI
; interfaccia mouse
; pesi singoli bit
CLOCKO	equ		2				; PORT C, bit 1 - clock out
DATAO	equ		1				; PORT C, bit 0 - data out
CLOCKI	equ		8				; PORT C, bit 3 - clock in
DATAI	equ		4				; PORT C, bit 2 - data in
; combinazioni output
CLDL	equ		0				; clock low, data low
CLDH	equ		1				; clock low, data high
CHDL	equ		2				; clock high, data low
CHDH	equ		3				; clock high, data high
MMASK	equ		$fc				; maschera scritture mouse: abbassa clock, data
MCLOCKL	equ		$fd				; abbassa clock
MDATAL	equ		$fe				; abbassa data

SERBUFL	equ		$30				; buffer seriale 48 caratteri

;
; locazioni di memoria
;
varBeg	equ		$9a				; inizio area variabili
zencIni	equ		varBeg			; stato iniziale zero encoder per la ricerca
flags	equ		zencIni+1		; flag per protocollo comunicazione
								; (NON COINCIDE ESATTAMENTECON LO STATO
								; ... RITORNATO DALLA SERIALE!! - B7,B6 diversi)
								; B7: dobbiamo rispondere
								; B6: trasmissione in corso per serUpdate
								; B5: ricerca zero encoder Y attiva
								; B4: ricerca zero encoder X attiva
								; B3: spare (zero)
								; B0-B2: codice ultimo messaggio ricevuto
temp16	equ		flags+1			; 16 bit temporanei per vari usi
targetX	equ		temp16+2		; destinazione X su 3 byte
setpX	equ		targetX+3		; setpoint corrente X su 3 byte
speedX	equ		setpX+3			; incremento tick/ciclo su 16 bit
mouseX	equ		speedX+2		; X letto dal mouse cumulato (3 byte)
outX	equ		mouseX+3		; output del controllo X (2 byte)
targetY	equ		outX+2			; destinazione Y su 3 byte
setpY	equ		targetY+3		; setpoint corrente Y su 3 byte
speedY	equ		setpY+3			; incremento tick/ciclo su 16 bit
mouseY	equ		speedY+2		; Y letto dal mouse cumulato (3 byte)
outY	equ		mouseY+3		; output del controllo X (2 byte)
sBuf	equ		outY+2			; buffer per seriale 48 CARATTERI
sNumCh	equ		sBuf+SERBUFL	; numero di caratteri letti (2 byte)
varEnd	equ		sNumCh+2		; fine area variabili (primo byte di stack = $EA)

;
; altre costanti
;
NCLEAR	equ		varEnd-varBeg	; numero variabili da azzerare a partire da varBeg
SYN		equ		$16				; codice ASCII SYN
MAXDEST	equ		2				; massimo destinatario per i messaggi
ME		equ		1				; il mio indirizzo (1 o 2; 0:controllore luci)

;
;
		ORG		$F800			; inizio E2PROM 68HC811E2
;
; inizializza porta C
; bit 0,1 output e open drain
; sporca a
;
PCinit	ldx		#H11REGS
		bset	PIOC,x #CWOM	; PORT C open drain
		ldaa	#CHDH			; bit C0,C1 out
		staa	H11DDRC			; inizializza default PORT C
		staa	H11PORTC		; CLOCK, DATA alti
		rts

;
; inizializza il mouse
; sporca a,b,x,y
; torna Z=0 se OK
;
MouseInit
		bsr		PCinit			; inizializza la porta
		ldx		#0				; ritardo iniziale (loop 64k cicli)
MI_0	dex
		bne		MI_0

MI_def	ldab	#$F6			; cmd DEFAULT
		jsr		SendMouse		; invia (non controlla, tanto c'e' recv)

MI_rd	jsr		RecvMouse
		beq		MI_err
		bcc		MI_rd			; loop finche' arriva qualcosa o errore
		cmpb	#$FA			; ACK ?
		bne		MI_def			; retry

MI_strt	ldab	#$F4			; cmd START
		jsr		SendMouse		; invia

MI_rd1	jsr		RecvMouse
		beq		MI_err
		bcc		MI_rd1
		cmpb	#$FA			; ACK ?
		bne		MI_strt			; retry
		ldaa	#$FF			; OK, esci con Z=0
MI_err	rts

;
; invia byte in b al mouse: ritorna flag Z=0 se OK
; sporca a,b,x,y
;
SendMouse
		ldaa	#CLDH			; CLOCK basso, DATA alto (req. to send)
		staa	H11PORTC
		ldx		#40				; (3) ritarda circa 100 us
SM_1	dex						; (3)
		bne		SM_1			; (3)
		anda	#MDATAL			; (2) abbassa DATA (start bit pronto)
		staa	H11PORTC		; (4)
		ldx		#5				; (3) ritarda circa 20 us (39 cicli)
SM_2	dex						; (3)
		bne		SM_2			; (3)
		oraa	#CHDL			; (2) alza CLOCK (fine req. to send, start bit pronto)
		staa	H11PORTC		; (4)
;
; loop di trasmissione dei bit, compresi start, e parita'
;
		ldy		#1				; contatore di parita' (init 1 per dispari)
		ldx		#8				; 8 bit da passare nel campo dati
SM_loop	lsra					; predisponi A per caricare il prossimo bit
		bsr		WaitMClL		; attendi clock basso dal mouse
		beq		SM_err			; timeout ? se si' esci con Z=1
		lsrb					; leggi un bit da B nel carry
		bcc		SM_L
		iny						; bit alto -> increm. conteggio parita'
SM_L	rola					; ... e caricalo in A (LSB=DATAO)
		staa	H11PORTC
		bsr		WaitMClH		; attendi clock alto dal mouse
		beq		SM_err			; timeout ? se si' esci con Z=1
		dex						; decrementa il contatore di bit
		bne	SM_nopa				; <8 o 9 cicli fatti
;
; 8 cicli fatti, predisponi il bit di parita' (dispari) e rientra nel ciclo
;
		xgdy					; contatore parita' (y) -> D
		ldaa	#CHDH			; ricarica A, B ora contiene Ybasso
		bra		SM_loop			; fai un ultimo ciclo
SM_nopa	cpx		#$FFFF			; fatti 9 cicli ?
		bne		SM_loop			; no, continua il loop per gli 8 bit
		bsr		WaitMClL		; attendi il clock basso del bit di parita'
		beq		SM_err
		oraa	#CHDH			; ora noi smettiamo di comandare DATA
		staa	H11PORTC		; ... -> lo mettiamo alto
		bsr		WaitMClH		; attendi il clock alto dopo il bit di parita'
		beq		SM_err
;
; aspetta un ultimo clock con DATA basso (-> ACK del mouse)
;
		bsr		WaitMClL
		beq		SM_err			; se timeout esci con Z=1
		ldaa	H11PORTC
		anda	#DATAI			; DATA deve essere basso
		eora	#DATAI			; inverti il bit per avere Z=1 su errore
		beq		SM_err
		bra		WaitMClH		; attendi l'ultimo clock alto ED ESCI. Se Z=0 tutto ok
;
; uscita in caso di errore
;
SM_err	ldaa	#CHDH			; ripristina la porta (-> CLOCK,DATA alti)
		staa	H11PORTC
		clra					; forza uscita con Z=1
		rts

;
; attendi una transizione di clock alto->basso dal mouse
; timeout circa 1 ms
; torna Z=0 se OK
; non sporca i registri
;
WaitMClL
		psha					; (3) salva a,x
		pshx					; (4)
		ldx		#133			; (3) contatore timeout
WMCL_hi	ldaa	H11PORTC		; (4) attendi clock basso
		anda	#CLOCKI			; (2)
		beq		WMCL_ok			; (3) ok, basso
		dex						; (3)
		bne		WMCL_hi			; (3) un loop 15 cicli -> 7.5 us
		bra		WMCL_ex			; (3) timeout, torna flag Z=1
WMCL_ok	inx						; (3) forza flag Z=0 per uscire con OK
WMCL_ex	pulx					; (4) recupera i registri
		pula					; (5)
		rts						; (5) se timeout, torna Z=1

;
; attendi una transizione di clock basso->alto dal mouse
; timeout circa 1 ms
; torna Z=0 se OK
; non sporca i registri
;
WaitMClH
		psha					; (3) salva a,x
		pshx					; (4)
		ldx		#133			; (3) contatore timeout
WMCH_lo	ldaa	H11PORTC		; (4) attendi clock alto
		anda	#CLOCKI			; (2)
		bne		WMCL_ex			; (3) ok, alto, torna flag Z=0
		dex						; (3)
		bne		WMCH_lo			; (3) loop a meno di timeout
		bra		WMCL_ex			; (3) esci con errore (Z=1)

;
; riceve byte dal mouse: ritorna C=1 e Z=0 se c'e' byte in b,
; C=0 e Z=0 se non c'e' niente, C=0 e Z=1 se errore
; sporca a,b,x,y
;
RecvMouse
		ldaa	H11PORTC		; (4) verifica se c'e' un byte pronto a partire
		tab						; (2) salva in b per non ricaricare poi
		anda	#CLOCKI			; (2) vogliamo clock basso
		beq		RM_start		; (3) ok, inizia ricezione
		clc						; (2) niente, torna C=0 e Z=0
		rts
RM_start
		andb	#DATAI			; il bit di start ha DATA basso
		bne		RM_err			; subito sbagliato ...
		bsr		WaitMClH		; attendi fine start bit
		beq		RM_err			; esci se timeout
		ldy		#0				; contatore di parita' (dispari)
		ldx		#9				; contatore di bit dati + parita'
RM_loop	bsr		WaitMClL		; aspetta CLOCK basso
		beq		RM_err			; esci se timeout
		ldaa	H11PORTC		; leggi lo stato di DATA
		lsra					; porta il bit in C con 3 shift
		lsra
		lsra
		bcc		RM_L			; se basso, non incrementa contatore parita'
		iny						; contatore parita' (conta anche il bit di par.)
RM_L	rorb					; carica in b (i bit sono dal meno signif.)
		bsr		WaitMClH		; aspetta CLOCK alto
		beq		RM_err			; solito test sul timeout
		dex						; decrementa contatore 8 bit
		bne		RM_loop			; cicla
		rolb					; recupera LSB perso leggendo il bit di parita'
;
; controllo di parita': ora y deve essere dispari -> LSB=1
; (perche' ha contato un numero dispari di uni partendo da 0)
;
		xgdy					; y basso ora e' in b
		andb	#1				; test LSB
		beq		RM_err			; zero -> pari -> errore
		xgdy					; recupera b (anche a che non serve)
		bsr		WaitMClL		; aspetta CLOCK del bit di stop
		beq		RM_err			; timeout ?
		ldaa	H11PORTC		; il bit di stop da mouse e' alto
		anda	#DATAI
		beq		RM_err			; bit basso -> errore
		bsr		WaitMClH		; termine del bit di stop (si potrebbe forse non aspettare)
		beq		RM_err			; timeout ?
		sec						; OK, esci con C=1 (e Z=0)
		rts
RM_err	clc						; errore, esci con C=0 e Z=1
		clra					; forza Z=1
		rts

;
; inizializza SPI
; sporca a
;
iniSpi	ldaa	#$2f			; SS alto, SCK basso, MOSI alto
		staa	H11PORTD		; inizializza la porta prima di programmarla in uscita
		ldaa	#$38			; SS, SCK, MOSI out, MISO, TXD, RXD in (non influenza 232)
		staa	H11DDRD
		ldaa	#$70			; SPI master, CPHA=0, CPOL=0, E/2 clock, DWOM
		staa	H11SPCR
		rts

;
; subroutine di scrittura su SPI
; scrive su DAC i due valori in a e b
; sporca a
; non fa polling SPSR, perche' col clock E/2
;   bastano 17-18 cicli per completare una scrittura SPI
;   N.B.: la lettura di SPSR riabilita una nuova scrittura,
;         e deve essere fatta dopo il completamento di quella precedente
;
wrSpi	pshx					; salva x
		psha					; salva a
		ldx  	#H11REGS
		bclr 	PORTD,x $20		; abbassa chip select

		ldaa	SPSR,x			; resetta SPIF per abilitare TX
		ldaa	#$09			; primo byte: comando load DAC A
		staa	SPDR,x

		bsr		dummy			; (6+5=11) ritardo per consentire TX
		pula					; (4) recupera il valore DAC A
		tst		SPSR,x			; (6) resetta SPIF per abilitare TX
		staa	SPDR,x			; (4) secondo byte: valore DAC A

		bsr		dummy			; (11)
		nop						; (2) ritardo addizionale
		bset	PORTD,x $20		; (7) alza chip select
		bclr 	PORTD,x $20		; (7) abbassa chip select

		ldaa	SPSR,x			; resetta SPIF
		ldaa	#$0A			; primo byte: comando load DAC B
		staa	SPDR,x

		bsr		dummy			; (11)
		nop						; (2) ritardo addizionale
		nop						; (2)
		tst		SPSR,x			; (6) resetta SPIF
		stab	SPDR,x			; (4) secondo byte: valore DAC B

		bsr		dummy			; (11)
		nop						; (2)
		bset	PORTD,x $20		; (7) alza chip select
		pulx
dummy	rts

;
; blocco di inizializzazione
;
Initialize
		jsr		MouseInit		; inizializza mouse
		jsr		iniSpi			; inizializza porta DAC
		bsr		serIni			; inizializza porta RS232
		ldx		#NCLEAR			; azzera le variabili da varBeg inclusa a varEnd esclusa
clLoop	clr		varBeg-1,x
		dex
		bne		clLoop
		ldx		#20000			; ritardo dopo init
I_0		dex						; cosi' il mouse legge correttamente
		bne		I_0				; ... il primo "right-click"
		rts

;
; inizializza la porta seriale
; sporca a
;
serIni
		ldaa	#TCLR
		oraa	#SCP13
;		oraa	#SCR8			; *NON* imposta la velocita' a 1200 baud
;		oraa	#SCR1			; *NON* imposta a 9600 baud
		oraa	#SCR2			; imposta a 4800 baud
		staa	H11BAUD
		ldaa	#TE
		oraa	#RE				; abilita la seriale
;		oraa	#RIE			; *NON* abilita le interruzioni da seriale
		staa	H11SCCR2
		clra
		staa	sNumCh			; azzera contatore caretteri letti
		staa	sNumCh+1
		rts

;
; scrive il byte in a sulla seriale
; non sporca i registri
;
serPut
		psha					; salva il byte
serWait	ldaa	H11SCSR			; attendi che la porta sia libera
		anda	#TDRE			; flag transmit data register empty
		beq		serWait
		pula					; recupera il byte
		staa	H11SCDR			; scrivilo sul data register
		rts

;
; legge la seriale in a
; torna C=1 se c'e' un dato pronto
;
serGet
		clc						; (2) assumi dato non pronto
		ldaa	H11SCSR			; (4) vedi se c'e' un byte pronto
		anda	#RDRF			; (2) flag receive buffer full
		beq		serExit			; (3) no, torna Z=1
		ldaa	H11SCDR			; (4) leggi il carattere
		sec						; (2) dato pronto
serExit	rts

;
; accumula caratteri da seriale
; non usa serGet perche' sarebbe troppo lenta
;
;serUpdate
;		ldaa	H11SCSR			; (4) vedi se c'e' un byte pronto
;		anda	#RDRF			; (2) flag receive buffer full
;		beq		serEx1			; (3) no
;		ldaa	H11SCDR			; (4) leggi il carattere
;		ldx		sNumCh			; (4) bufferizza
;		cpx		#SERBUFL		; (4) overflow?
;		bhs		serEx1			; (3) si', butta via
;		staa	sBuf,x			; (4) no, salva il carattere
;		inx						; (3)
;		stx		sNumCh			; (4) aggiorna la lunghezza del buffer
;serEx1	rts

;
; accumula/trasmette caratteri da/su seriale
; non usa serGet/serPut perche' sarebbe troppo lenta
;
serUpdate
		ldaa	H11SCSR			; (4) carica registro stato seriale
		brset	flags #$40 updTx	; (6) se stiamo trasmettendo vai a updTx
		anda	#RDRF			; (2) flag receive buffer full
		beq		serEx1			; (3) no
		ldaa	H11SCDR			; (4) leggi il carattere
		ldx		sNumCh			; (4) bufferizza
		cpx		#SERBUFL		; (4) overflow?
		bhs		serEx1			; (3) si', butta via
		staa	sBuf,x			; (4) no, salva il carattere
		inx						; (3)
		stx		sNumCh			; (4) aggiorna la lunghezza del buffer
serEx1	rts
updTx	anda	#TDRE			; (2) flag transmit data register empty
		beq		serEx1			; (3) no, esci
		ldx		sNumCh			; (4) carica numero caratteri da trasmettere
		ldaa	sBuf-1,x		; (4) leggi (il buffer e' caricato alla rovescia)
		staa	H11SCDR			; (4) scrivilo sul data register
		dex						; (3) aggiorna la lunghezza buffer
		stx		sNumCh			; (4)
		bne		serEx1			; (3) se finito, spegni flag TX
		bclr	flags #$40		; (6)
		rts

;
; lettura coordinate mouse e accumulo su deltaX, deltaY
; LA LETTURA E' BLOCCANTE (esce eventualmente con timeout), perche'
;   non gestibile un polling a livello superiore per via della
;   criticita' della temporizzazione
; il messaggio da mouse e' di 3 byte
; torna C=1 se lettura effettuata e OK, C=0 se errore
; sporca a,b,x,y
; il primo loop (loop MRloop) e' critico, infatti il primo clock basso
;   del mouse dura solo 45us (90 cicli), e deve essere rilevato da RecvMouse
; La routine aggiorna il buffer di ricezione seriale durante le attese
;
MouseRead
		jsr		serUpdate		; dai un'occhiata alla seriale
MRloop	jsr		RecvMouse		; (24/~1000) cerca il primo byte
		beq		MRErr1			; (3) errore, visualizza su PB7 ed esci
		bcs		MRbyte1			; (3) ok, interpreta il byte 1
		bsr		serUpdate		; (26/54) non c'e' niente, dai un'occhiata alla seriale
		bra		MRloop			; (3) ... e poi aspetta (pero' cosi' non c'e' il timeout!)
MRbyte1	andb	#$0C			; ok, i bit 2,3 devono essere 0,1
		cmpb	#$08
		bne		MRErr2			; errore, visualizza ed esci (aiuta resync)
		bsr		serUpdate		; (20/46) dai un'occhiata alla seriale prima del byte 2
MRloop2	jsr		RecvMouse		; cerca il secondo byte
		beq		MRErr1			; errore, visualizza su PB7 ed esci
		bcc		MRloop2			; lo aspettiamo (tanto c'e' il timeout)
		ldx		#mouseX			; carica l'indirizzo di mouseX (accumulatore a 24 bit)
		bsr		AddB_24			; somma il risultato in B ai 24 bit puntati da X
		bsr		serUpdate		; (20/46) dai un'occhiata alla seriale prima del byte 3
MRloop3	jsr		RecvMouse		; cerca il secondo byte
		beq		MRErr1			; errore, visualizza su PB7 ed esci
		bcc		MRloop3			; lo aspettiamo
		ldx		#mouseY			; carica l'indirizzo di mouseY (accumulatore a 24 bit)
		bsr		AddB_24			; somma il risultato in B ai 24 bit puntati da X
		sec						; segnala che abbiamo letto
MRExit	rts
MRErr1	ldaa	H11PORTB		; accendi LED B7
		oraa	#$80
		bra		MRErrEx
MRErr2	ldaa	H11PORTB		; accendi LED B6
		oraa	#$40
MRErrEx	staa	H11PORTB
		clc						; esci C=0 per segnalare errore
		rts

;
; somma 24+16 -> 24 con segno
; il dato a 24 bit e' puntato da X (MSB prima)
; il dato a 16 bit e' contenuto in D
; sporca a
; ATTN: clra azzera C e non si puo' usare al posto di ldaa #0 !!!
;
AddD_24	tsta					; verifica il segno del byte piu' signif.
		bmi		AdNeg			; D<0 -> va esteso con $FF
		addd	1,x				; somma i 16 bit bassi a D
		std		1,x				; salvali
		ldaa	#0				; somma C agli 8 bit alti (estensione 0)
AdHi	adca	0,x
		staa	0,x
		rts
AdNeg	addd	1,x				; come sopra per i 16 bit bassi
		std		1,x
		ldaa	#$ff			; somma l'estensione del numero negativo + C
		bra		AdHi

;
; somma 24+8 -> 24 con segno
; il dato a 24 bit e' puntato da X (MSB prima)
; il dato a 8 bit e' contenuto in B
; sporca a
;
AddB_24	clra					; calcola l'estensione di segno
		tstb					; se negativo occorre 0xFF
		bpl		AdbOk
		ldaa	#$ff
AdbOk	bra		AddD_24			; esci attraverso la somma 16+24

;
; aggiorna setpoint X o Y
; input: X punta all'inizio della sequenza di variabili:
;        target finale (3 byte), setpoint corrente (3 byte), speed (2 byte), mouse (3 byte)
; assume che speed sia nel range 0..32767
; modifica a,b,temp16
;
SetpUpd ldd		1,x				; calcola target-setpoint
		subd	4,x				; 16 bit bassi
		std		temp16			; salva su buffer temporaneo a 16 bit
		ldaa	0,x				; 8 bit alti
		sbca	3,x
		beq		SUhi_0			; parte alta della diff. zero -> e' nulla o positiva piccola
		bpl		SUincV			; diff. positiva non piccola -> occorre incrementare il setpoint
		cmpa	#$ff			; se parte alta $ff -> negativa piccola
		bne		SUdecV			; negativa non piccola -> occorre decrementare il setpoint
		ldd		temp16			; diff. neg. piccola, prova ad aggiungere la velocita'
		bpl		SUdecV			; il numero visto a 16 bit e' positivo -> neg. grande
		addd	6,x				; ok <0 e >=-32768, somma la velocita'
		bmi		SUdecV			; se resta negativa il decremento ci vuole tutto
SUsetV	ldd		1,x				; siamo vicini, poni setpoint = target
		std		4,x
		ldaa	0,x
SUex1	staa	3,x
		rts
SUhi_0	ldd		temp16			; diff. positiva piccola o nulla, prova a sottrarre la velocita'
		bmi		SUincV			; se >32768 l'incremento di velocita' ci vuole tutto
		subd	6,x
		bpl		SUincV			; se ancora positiva l'incremento di posizione ci vuole tutto
		bra		SUsetV			; siamo vicini, allinea setpoint al target
SUincV	ldd		6,x				; setpoint += speed
		addd	4,x
		std		4,x
		ldaa	#0				; la parte alta di speed e' zero
		adca	3,x
		bra		SUex1
SUdecV	ldd		4,x				; setpoint -= speed
		subd	6,x
		std		4,x
		ldaa	3,x				; la parte alta di speed e' zero
		sbca	#0
		bra		SUex1

;
; calcola setpoint-mouse e satura il risultato su 16 bit -> d
; input: X punta all'inizio della sequenza di variabili:
;        target finale (3 byte), setpoint corrente (3 byte), speed (2 byte), mouse (3 byte)
; modifica a,b,temp16
;
PosErr	ldd		4,x				; parte bassa
		subd	9,x
		std		temp16			; salva su buffer temporaneo
		ldaa	3,x				; 8 bit superiori
		sbca	8,x
		beq		PEpos			; parte alta zero -> la diff. e' positiva piccola (o zero)
		bpl		PEsatP			; parte alta positiva -> carica il massimo positivo su 16 bit
		cmpa	#$ff			; parte alta $ff -> la diff. e' negativa piccola
		bne		PEsatN
		ldd		temp16			; carica la diff. da temp16
		bpl		PEsatN			; positiva non va bene, satura in negativo
		rts						; ok, negativa tra $8000 e $ffff
PEsatN	ldd		#$8000			; carica il massimo negativo ed esci
		rts
PEpos	ldd		temp16			; carica la diff. da temp16
		bmi		PEsatP			; negativa qui non va bene, satura in positivo
		rts						; ok, positiva < $7fff
PEsatP	ldd		#$7fff			; carica il massimo positivo ed esci
		rts

;
; applica i comandi outX, outY ai motori
;  controllando il segno
;  sporca a,b,temp16
;
ApplyOut
		ldd		outX			; carica outX
		bpl		cPosiX			; positivo -> salta
		cmpa	#$ff			; negativo oltre -256 ?
		beq		cNegSmX			; no
		ldab	#0				; si', massima velocita' negativa
cNegSmX	eorb	#$ff			; complementa perche' vogliamo il modulo della velocita'
		ldaa	H11PORTB		; accendi bit di direzione motore
		oraa	#1				; negativo, accendi bit di direzione motore
		anda	#$f7			; spegni bit complementare per "enable switching"
		bra		cEndX
cPosiX	tsta					; se la parte alta e' zero -> positivo piccolo
		beq		cPosSmX			; positivo <=255
		ldab	#$ff			; positivo grande, satura
cPosSmX	ldaa	H11PORTB		; positivo, spegni bit di direzione motore
		anda	#$FE
		oraa	#8				; accendi bit complementare per "enable switching"
cEndX	staa	H11PORTB
		stab	temp16			; salva il comando motore X
		ldd		outY			; carica outY
		bpl		cPosiY			; positivo -> salta
		cmpa	#$ff			; negativo oltre -256 ?
		beq		cNegSmY			; no
		ldab	#0				; si', massima velocita' negativa
cNegSmY	eorb	#$ff			; complementa perche' vogliamo il modulo della velocita'
		ldaa	H11PORTB		; accendi bit di direzione motore
		oraa	#2				; negativo, accendi bit di direzione motore
		anda	#$ef			; spegni bit complementare per "enable switching"
		bra		cEndY
cPosiY	tsta					; se la parte alta e' zero -> positivo piccolo
		beq		cPosSmY			; positivo <=255
		ldab	#$ff			; positivo grande, satura
cPosSmY	ldaa	H11PORTB		; positivo, spegni bit di direzione motore
		anda	#$FD	
		oraa	#$10			; accendi bit complementare per "enable switching"
cEndY	staa	H11PORTB
		ldaa	temp16			; ricarica il comando al DAC A (B e' ok)
		jmp		wrSpi			; esci scrivendo su DAC

;
; azzera setpoint, target speed e mouse X
; sporca x
;
zerSmX	ldx		#11
zerXlp	clr		targetX-1,x
		dex
		bne		zerXlp
		rts

;
; azzera setpoint, target speed e mouse Y
; sporca x
;
zerSmY	ldx		#11
zerYlp	clr		targetY-1,x
		dex
		bne		zerYlp
		rts

;
; compatta il buffer rx seriale mangiando B caratteri
; sporca A,B,X,Y
;
serEat	tba						; copia numero di byte da mangiare in A
		nega					; A = -A
		adda	sNumCh+1		; A = (numero ch) - (numero ch da mangiare)
		staa	sNumCh+1		; aggiorna numero ch (usata solo parte bassa)
		bne		eatEat			; <>0 -> c'e' pappa
		rts
eatEat	ldaa	#0				; copia numero ch da mangiare in Y
		xgdy
		ldx		#0
eatLoop	ldaa	sBuf,y			; copia indietro di quanto serve
		staa	sBuf,x
		iny						; incrementa gli indici
		inx
		cpx		sNumCh			; finito ?
		bne		eatLoop
		rts

;
; leggi seriale/decodifica messaggio
;
serDecode
		ldaa	sNumCh+1		; intanto devono esserci almeno 3 caratteri
								; un messaggio ne ha di piu', ma vogliamo vuotare
								; il buffer per tempo se ci sono errori
		cmpa	#3				; SYN+dest+len
		bhs		deChars
deExit	rts						; buffer vuoto o insufficiente, esci
deChars	ldaa	sBuf			; controlla l'iniziatore
		cmpa	#SYN
		beq		deInOk
deEat1	ldab	#1				; sbagliato, mangia il carattere
deEatN	bsr		serEat
		bra		serDecode		; continua coi successivi
deInOk	ldaa	sBuf+1			; ora controlla il destinatario
		cmpa	#MAXDEST
		bhi		deEat1			; fuori range -> mangia iniziatore e riprova
								; 1:usa test senza segno
								; 2:non mangia due ch, il secondo potrebbe essere SYN!
		ldab	sBuf+2			; lunghezza rimanente in B
		cmpb	#SERBUFL-3		; confronta col massimo ammissibile (3 ch gia' usati)
		bhi		deEat1			; fuori range -> mangia iniziatore e riprova
		addb	#3				; calcola la lunghezza totale
		cmpb	sNumCh+1		; abbiamo l'intero messaggio?
		bhi		deExit			; no, riproviamo al prossimo giro
		cmpa	#ME				; il messaggio e' per noi?
		bne		deEatN			; no, mangialo tutto e riprova
		decb					; si, controlla il checksum
		decb					; B=#ch per test checksum (tot-2)
		ldaa	#0				; copia in X
		xgdx
		ldaa	sBuf+1,x		; carica il checksum in a
deCkLp	eora	sBuf,x			; calcola xor all'indietro
		dex
		bne		deCkLp			; salta l'indice zero (SYN non incluso nel calcolo)
		tsta					; deve essere zero
		beq		deCkOk
deFlush	ldab	sBuf+2			; sbagliato, butta via tutto
		addb	#3				; ricalcola la lunghezza totale
		bra		deEatN			; mangia tutto e riprova
deCkOk	ldab	sBuf+2			; calcola B=lunghezza del campo dati
		subb	#2
		ldaa	sBuf+3			; A=codice messaggio
		cmpa	#2				; 0-1 ?
		bhs		deNo01
		cmpb	#2				; campo dati 2 byte
		bne		deFlush			; sbagliato, butta via tutto
		tsta					; distingui 0/1
		bne		deIs1
		bset	flags #$90		; accendi flag risposta + ricerca zero encoder X + codice 0
;
; inizio routine zero encoder: leggi lo stato iniziale dello zero in zencIni per
;  l'asse desiderato, poi imposta una posizione corrente zero (target e mouse) e
;  un setpoint molto grande (MSB=7F o 80) secondo lo stato iniziale dello zero enc
;
		jsr		zerSmX
		bclr	zencIni #2		; leggi stato iniziale zero encoder X in zencIni
		ldx		#H11REGS
		brclr	PORTA,x #2 deZex0	; ... senza toccare il bit Y
		bset	zencIni #2
		ldaa	#$80			; imposta targetX nagativo molto grande
		bra		deZex1
deZex0	ldaa	#$7f			; imposta targetX positivo molto grande
deZex1	staa	targetX
		ldd		sBuf+4			; leggi speedX da sBuf
		std		speedX
deFlu1	ldab	sBuf+2			; flush senza ritornare a serDecode
		addb	#3				; ... (elaboriamo un solo messaggio alla volta)
		jmp		serEat			; esci attraverso serEat
deIs1	bset	flags #$a1		; accendi flag risposta + ricerca zero encoder Y + codice 1
		jsr		zerSmY			; routine zenc-Y come per l'asse X
		bclr	zencIni #1		; leggi stato iniziale zero encoder Y in zencIni
		ldx		#H11REGS
		brclr	PORTA,x #1 deZey0	; ... senza toccare il bit X
		bset	zencIni #1
		ldaa	#$80			; imposta targetY negativo molto grande
		bra		deZey1
deZey0	ldaa	#$7f			; imposta targetY positivo molto grande
deZey1	staa	targetY
		ldd		sBuf+4			; leggi speedY dai dati
		std		speedY
		bra		deFlu1			; esci
deNo01	cmpa	#4				; 2-3 ?
		bhs		deNo23
		cmpb	#5				; 5 byte campo dati
		bne		deFlush			; lunghezza sbagliata, butta via
		cmpa	#3				; distingui 2-3
		beq		deIs3
		bset	flags #$82		; accendi flag risposta + codice 2
		bclr	flags #$10		; annulla ricerca zero encoder X
		ldd		sBuf+4			; copia dati su targetX e speedX
		std		targetX
		ldaa	sBuf+6
		staa	targetX+2
		ldd		sBuf+7
		std		speedX
		bra		deFlu1			; esci
deIs3	bset	flags #$83		; accendi flag risposta + codice 3
		bclr	flags #$20		; annulla ricerca zero encoder Y
		ldd		sBuf+4			; copia dati su targetY e speedY
		std		targetY
		ldaa	sBuf+6
		staa	targetY+2
		ldd		sBuf+7
		std		speedY
		bra		deFlu1			; esci
deNo23	bne		degoFlu			; rimane solo il codice 4. Se errato mangia il messaggio
		tstb					; campo dati nullo
		bne		degoFlu
		bset	flags #$84		; accendi flag risposta + codice 4
		bra		deFlu1			; esci
degoFlu	jmp		deFlush			; helper per salto troppo lontano

;
; invia messaggio seriale
; la risposta e' uguale per tutti i messaggi di interrogazione (almeno per ora)
; il buffer seriale va riempito all'indietro (ultimo carattere
;   in posizione sBuf, primo in posizione sBuf+sNumCh-1)
; sporca a,b,x
;
serAnswer
		ldx		#10				; lunghezza 10 byte
		stx		sNumCh			; salvala (sNumCh DOVREBBE essere zero, half duplex)
		ldaa	#SYN			; iniziatore
		staa	sBuf-1,x		; scrivilo alla fine del buffer
		dex						; ... e aggiorna l'indice
		ldaa	#8				; lunghezza (dopo SYN e questo byte)
		staa	sBuf-1,x
		dex
		ldaa	flags			; costruisci il byte di stato
		anda	#$3f			; codice messaggio + spare bit + stato ricerca z-enc
		ldab	H11PORTA		; leggi lo stato degli input di zero encoder
		rorb					; vogliamo portare in B6-B7 invertendo di posizione
		andb	#1				; preserva solo il bit zero encoder X
		beq		anZeX0
		ldab	#$80			; sposta il bit su B7
anZeX0	rorb					; ora ZEY->B7, ZEX->B6
		aba						; byte di stato completo
		staa	sBuf-1,x		; ZEY.ZEX.RZEY.RZEX.SPARE(0).COD2.COD1.COD0
		dex
		ldaa	mouseX			; copia la posizione X
		staa	sBuf-1,x
		dex
		ldaa	mouseX+1
		staa	sBuf-1,x
		dex
		ldaa	mouseX+2
		staa	sBuf-1,x
		dex
		ldaa	mouseY			; copia la posizione Y
		staa	sBuf-1,x
		dex
		ldaa	mouseY+1
		staa	sBuf-1,x
		dex
		ldaa	mouseY+2
		staa	sBuf-1,x
		ldx		#8				; calcola il checksum
		ldaa	#0
anCkLp	eora	sBuf,x			; 8 byte (esclusi SYN e checksum stesso)
		dex
		bne		anCkLp
		staa	sBuf			; scrivi il checksum alla fine msg (inizio del buffer)
		bclr	flags #$87		; spegni flag risposta e azzera codice messaggio rx
		bset	flags #$40		; accendi flag di trasmissione in corso
		rts

;
; un ciclo del loop di controllo
; X: encoder LENTO: poca riduzione, meno di 2000 impulsi / giro asse planetario
; Y: encoder VELOCE: molta riduzione, circa 110000 impulsi / giro asse planetario
; l'encoder Y e' critico, perche' puo' superare la massima frequenza ammessa dal
;   controllore mouse se il motore e' eccitato al massimo a lungo. Per limitare
;   questo effetto non aggiorniamo il setpoint se prima di farlo l'errore e' gia'
;   tale da eccitare il motore al massimo in una direzione. L'algoritmo e' applicato
;   anche per l'encoder X, ma permettendo di superare la massima eccitazione, perche'
;   in questo caso la frequenza degli impulsi e' sempre bassa. L'algoritmo e' utile
;   in generale perche' evita che il setpoint si allontani troppo dalla posizione corrente.
;
Control ldx		#H11REGS		; vedi se sono scaduti i 65.536 ms del ciclo
		brclr	TFLG1,x	#$40 CNoUpd	; ... di aggiornamento setpoint
		ldd		TOC2,x			; ok, programma nuovo aggiornamento
		addd	#8192			; 8192*(8 us) = 65536 us
		std		TOC2,x
		bset	TFLG1,x #$40	; spegni il flag OC2 del timer
;
; esegui aggiornamento dei setpoint
;
		ldx		#targetX		; comincia con X
		jsr		PosErr			; calcola errore posizione SENZA aggiornare il setpoint
		cpd		#-512			; se errore <-512 o >+512 salta l'aggiornamento setpoint
		blt		CSkpUpX
		cpd		#512
		bgt		CSkpUpX
		jsr		SetpUpd			; aggiorna setpoint X
CSkpUpX	ldx		#targetY		; passa a Y
		jsr		PosErr			; calcola errore posizione SENZA aggiornare il setpoint
		cpd		#-1024			; se errore <-1024 o >+1024 salta l'aggiornamento setpoint
		blt		CNoUpd
		cpd		#1024
		bgt		CNoUpd
		jsr		SetpUpd			; aggiorna setpoint Y
CNoUpd	ldx		#targetX
		jsr		PosErr			; calcola l'errore di posizione X
		aslb					; moltiplica per 4 -> max +/-2048
		rola
		aslb
		rola
		std		outX
;
; compensa banda morta +/-24 circa dell'attuatore
;
		bmi		CXNeg
		beq		CXZero
		addd	#24
		bra		CXAdd
CXNeg	subd	#24
CXAdd	std		outX			; ripeti out (il primo e' servito ad aggiornare i flag)
CXZero	ldx		#targetY		; aggiorna setpoint Y
		jsr		PosErr			; calcola l'errore di posizione Y
		asra					; applica guadagno 1/4 -> max +/-256
		rorb
		asra
		rorb
		std		outY
		bmi		CYNeg			; compensa banda morta come per X
		beq		CYZero
		addd	#24
		bra		CYAdd
CYNeg	subd	#24
CYAdd	std		outY
CYZero	jmp		ApplyOut		; out su DAC

;
; gestione zero encoder
; Se la ricerca e' attiva, attende una transizione del bit di zero
;  dell'encoder. Una volta trovata disattiva la ricerca e azzera
;  setpoint, target, mouse, speed
; sporca a,x
;
HandleZenc
		ldaa	H11PORTA		; leggi i valori correnti degli zero encoder
		eora	zencIni			; combinali coi valori iniziali
		brclr	flags #$10 hzNoX	; ricerca X attiva?
		bita	#2				; bit acceso -> stato cambiato
		beq		hzNoX			; stato invariato, passa ad Y
		bclr	flags #$10		; annulla ricerca zero encoder X
		jsr		zerSmX			; azzera setpoint, target, mouse, speed X

hzNoX	brclr	flags #$20 hzNoY	; ricerca Y attiva?
		bita	#1				; bit acceso -> stato cambiato
		beq		hzNoY			; stato invariato, esci
		bclr	flags #$20		; annulla ricerca zero encoder Y
		jmp		zerSmY			; azzera setpoint, target, mouse, speed Y ed esci

hzNoY	rts

;
; locazione di reset
; inizializza i dispositivi e poi esegui il loop di controllo
; Il settaggio del prescaler del timer va fatto entro i primi 64
;   cicli di clock, pertanto precede le routine principali di
;   inizializzazione.
;
reset_v	lds		#$ff
		ldaa	#$3				; prescaler 16 -> timer tick 8.0 us
		staa	H11TMSK2
		jsr		Initialize		; inizializza E2LIB e azzera variabili
		bsr		tm_init			; inizializza timer e attiva interrupt
;
; loop di controllo
;
lpLoop	jsr		MouseRead		; aggiorna mouseX, mouseY
		jsr		serUpdate		; controlla la seriale
		bsr		HandleZenc		; gestisci zero encoder
		jsr		Control			; esegui controllo motori
		jsr		serUpdate		; ultimo controllo alla seriale per un po'
		brset	flags #$40 lpNoDec	; no decodifica se trasmissione in corso
		jsr		serDecode		; vedi se ci sono messaggi in arrivo
lpNoDec	brclr	flags #$80 lpNoAns	; dobbiamo rispondere se flags:7 e' acceso
		jsr		serAnswer		; prepara la risposta (e carica in buffer)
lpNoAns	ldd		H11TCNT			; retriggera interrupt da timer tra 32.768 ms
		addd	#4096
		std		H11TOC1
		bra		lpLoop			; cicla

;
; inizializzazione servizi timer
;
tm_init	ldd		H11TCNT			; primo interrupt tra 32.768 ms (salvo altri comandi)
		addd	#4096			; 4096*(8 us) = 32768 us
		std		H11TOC1			; imposta output compare
		ldaa	#$80			; Output Compare 1 (OC1) interrupt enable
		staa	H11TMSK1
		ldaa	#$80			; spegni interrupt flag
		staa	H11TFLG1
		ldd		H11TCNT			; timeout OC2 ogni 8192 cicli (65.536 ms)
		addd	#8192
		std		H11TOC2			; no interrupt in questo caso
		cli						; abilita gli interrupt
		rts


;
; routine di servizio interrupt del timer (OC1)
; chiamata se sono passati 32.768 ms dall'ultimo ciclo di controllo
;   (significa che siamo bloccati in MouseRead perche' gli encoder sono fermi)
; sblocca il controllore mouse mediante un toggle del right-click
; la stessa cosa non funziona se fatta ad ogni ciclo di controllo, probabilmente
;   perche' il controllore mouse implementa il debounce. Richiederebbe
;   di controllare lo stato del bottone nel messaggio mouse, e invertirlo. In
;   ogni caso cosi' e' piu' sicuro che si sblocchi (se necessario si ripete
;   dopo altri 32.768 ms).
;
tm_int	psha					; salva a,b
		pshb
		ldd		H11TCNT			; incrementa output compare (32.768 ms da adesso)
		addd	#4096			; 4096*(8 us) = 32.768 ms
		std		H11TOC1
		ldaa	#$80			; spegni interrupt flag
		staa	H11TFLG1
		pulb					; recupera b per guadagnare tempo
		ldaa	H11PORTB		; forza un toggle del right-click mouse (collegato a PB2)
		eora	#4				; cosi' forziamo un messaggio mouse
		staa	H11PORTB
		pula					; recupera a ed esci
		rti

;
;
; vettori di interrupt/reset
;
;
		org		$ffe8
		dw		tm_int			; timer output compare 1

		org		$FFF6
		dw		reset_v			; software interrupt
		dw		reset_v			; illegal opcode
		dw		reset_v			; COP watchdog timeout
		dw		reset_v			; clk monitor failure
		dw		reset_v			; reset
		end
