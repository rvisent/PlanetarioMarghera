# -*- coding: utf-8
from visual import *
import serial

dbg_verbose=False

# variabili globali del modulo
_ser_buffer=""

class _opmode:
    decl = 0
    decl_spd = 0
    decl_target = 0
    ascr = 0
    ascr_spd = 0
    ascr_target = 0
    prec = 0
    prec_spd = 0
    prec_target = 0

class _stat:
    decl_zero = 0 # flag di zero
    ascr_zero = 0
    prec_zero = 0
    decl_enc = -3500
    ascr_enc = 1000
    prec_enc = 1500
    supernova = 0 # luci
    p_nebula = 0
    nuove_s = 0
    a_libero = 0
    stelle = 0
    sole = 0
    luna = 0
    cerchio_orario = 0
    libero_3 = 0
    libero_2 = 0
    p_cardinali = 0
    libero_1 = 0
    luce_sala = 0
    cerchio_polare = 0
    declinazione = 0
    pianeti = 0
    alba = 0
    tramonto = 0
    giorno = 0


def simpla_msg(data):
    global _ser_buffer, _opmode, _stat

    # appendi nuovi dati da seriale
    _ser_buffer += data
    
    # cerca un messaggio valido
    while True:
        # cerca SYN (torna -1 se non c'e')
        SYNpos = _ser_buffer.find(chr(0x16))
        if SYNpos>0:
            # se oltre posizione zero, compatta il buffer
            _ser_buffer = _ser_buffer[SYNpos:]
        elif SYNpos<0:
            return (False, "")
        
        # il messaggio più corto è di 5 byte
        if len(_ser_buffer) < 5:
            return (False, "")
        
        # se il destinatario non è valido, elimina solo il SYN e riprova
        dst_field = ord(_ser_buffer[1])
        if dst_field > 2:
            print "bad dst field %d" % (dst_field, )
            _ser_buffer = _ser_buffer[1:]
            continue
        
        # di sicuro abbiamo il byte con la lunghezza, controlla che sia in range
        len_field = ord(_ser_buffer[2])
        if len_field < 2 or len_field > 22:
            # se non valido elimina solo il SYN e riprova
            print "bad len field %d" % (len_field, )
            _ser_buffer = _ser_buffer[1:]
            continue
            
        # leggi il codice e controlla validità e coerenza con gli altri dati
        code_field = ord(_ser_buffer[3])
        bErr = False
        if dst_field==0:
           if (code_field==1 and len_field!=22) or ((code_field==2 or code_field==3) and len_field!=2):
               bErr = True
        else:
           if ((code_field==0 or code_field==1) and len_field!=4) or \
             ((code_field==2 or code_field==3) and len_field!=7) or \
             (code_field==4 and len_field!=2) or code_field>4:
               bErr = True
        if bErr:
            # elimina solo il SYN e riprova
            print "bad msg dst %d len %d code %d" % (dst_field, len_field, code_field)
            _ser_buffer = _ser_buffer[1:]
            continue

        # poi controlla che i dati ci siano tutti
        if len(_ser_buffer) < len_field+3:
            # dati insufficienti, riprova più tardi
            return (False, "")
        
        # estrai il messaggio, salta il SYN 
        msg = _ser_buffer[1:len_field+3]
            
        # checksum
        ck = 0
        for c in msg:
            ck ^= ord(c)
        if ck:
            print "checksum err"
            # riprova saltando SYN
            _ser_buffer = _ser_buffer[1:]
            continue

        # messaggio valido, esci da while
        break

    # ora compatta (o vuota) il buffer originale
    if (len(_ser_buffer) > len_field+3):
        _ser_buffer = _ser_buffer[len_field+3:]
    else:
        _ser_buffer = ""
        
    # decodifica
    bAnswer = False
    if dst_field == 0:
        # controllore luci
        if code_field==1:
            if len_field != 22:
                print "controller 0, code 1: bad len %d, expected 22" % (len_field, )
                return (False, "")
            # riempi i campi di _stat
            _stat.supernova = ord(msg[3])*256+ord(msg[4])
            _stat.p_nebula = ord(msg[5])*256+ord(msg[6])
            _stat.nuove_s = ord(msg[7])*256+ord(msg[8])
            _stat.a_libero = ord(msg[9])*256+ord(msg[10])
            _stat.stelle = ord(msg[11])*256+ord(msg[12])
            _stat.sole = ord(msg[13])*256+ord(msg[14])
            _stat.luna = ord(msg[15])*256+ord(msg[16])
            _stat.cerchio_orario = ord(msg[17])*256+ord(msg[18])
            bfield = ord(msg[19])
            _stat.libero_3 = (bfield & 0x80)==0x80
            _stat.libero_2 = (bfield & 0x40)==0x40
            _stat.p_cardinali = (bfield & 0x20)==0x20
            _stat.libero_1 = (bfield & 0x10)==0x10
            _stat.luce_sala = (bfield & 0x08)==0x08
            _stat.cerchio_polare = (bfield & 0x04)==0x04
            _stat.declinazione = (bfield & 0x02)==0x02
            _stat.pianeti = (bfield & 0x01)==0x01
            _stat.alba = ord(msg[20])
            _stat.tramonto = ord(msg[21])
            _stat.giorno = ord(msg[22])
            # no risposta

        elif code_field==2:
            if len_field != 2:
                print "controller 0, code 2: bad len %d, expected 2" % (len_field, )
                return (False, "")
            # ack, pacchetto vuoto come risposta
            ans_len = 1
            ans_payload = ""
            bAnswer = True
        
        elif code_field==3:
            if len_field != 2:
                print "controller 0, code 3: bad len %d, expected 2" % (len_field, )
                return (False, "")
            # version, pacchetto con un byte di risposta
            ans_len = 2
            ans_payload = chr(1)    # TBV
            bAnswer = True

        else:
            print "controller 0: bad code %d" % (code_field, )
            return (False, "")

    elif dst_field < 3:
        # controllori motori
        if code_field==0 or code_field==1:
            # ricerca zero
            if len_field != 4:
                print "controller %d, code %d: bad len %d, expected 4" % (dst_field,  code_field, len_field)
                return (False, "")
            speed = ord(msg[3])*256+ord(msg[4])
            if dst_field==1 and code_field==0:
                _opmode.decl = 1    # ricerca zero declinazione
                _opmode.decl_spd = speed
            elif dst_field==1 and code_field==1:
                _opmode.ascr = 1    # ricerca zero ascensione retta
                _opmode.ascr_spd = speed
            elif dst_field==2 and code_field==1:
                _opmode.prec = 1    # ricerca zero precessione
                _opmode.prec_spd = speed
            else:
                print "unexpected zero command for controller %d, code %d" %(dst_field, code_field)
                return (False, "")
        elif code_field==2 or code_field==3:
            # posizionamento
            if len_field != 7:
                print "controller %d, code %d: bad len %d, expected 7" % (dst_field,  code_field, len_field)
                return (False, "")
            target = ord(msg[3])*65536+ord(msg[4])*256+ord(msg[5])
            # gestisci target negativi
            if target>=0x800000:
                target=target-0x1000000
            speed = ord(msg[6])*256+ord(msg[7])
            # forza velocità positiva
            if speed>=32768:
                speed=65536-speed
            if dst_field==1 and code_field==2:
                _opmode.decl = 2    # goto declinazione
                _opmode.decl_target = target
                _opmode.decl_spd = speed
            elif dst_field==1 and code_field==3:
                _opmode.ascr = 2    # goto ascensione retta
                _opmode.ascr_target = target
                _opmode.ascr_spd = speed
            elif dst_field==2 and code_field==3:
                _opmode.prec = 2    # goto precessione
                _opmode.prec_target = target
                _opmode.prec_spd = speed
#                print "$$$target %d" % (target,)
            else:
                print "unexpected goto command for controller %d, code %d" % (dst_field, code_field)
                return (False, "")
        elif code_field==4:
            # richiesta di stato
            if len_field != 2:
                print "controller %d, code %d: bad len %d, expected 2" % (dst_field,  code_field, len_field)
                return (False, "")
        else:
            print "controller %d, bad code %d", (dst_field,  code_field)
            return (False, "")
            
        # se passiamo di qui, esegui un ciclo di controllo extra per aggiornare i valori
        simpla_motion()
        
        # la risposta è la stessa per tutti i messaggi dei controllori motori, cambia la scelta dei dati
        if dst_field==1:
            zx = _stat.decl_zero
            zy = _stat.ascr_zero
            rzx = (not _stat.decl_zero) and (_opmode.decl == 1) # ricerca in corso
            rzy = (not _stat.ascr_zero) and (_opmode.ascr == 1)
            encx = _stat.decl_enc
            ency = _stat.ascr_enc
        else:
            zx = False
            zy = _stat.prec_zero
            rzx = False
            rzy = not _stat.prec_zero and (_opmode.prec == 1) # ricerca in corso
            encx = 0
            ency = _stat.prec_enc
#            print "$$$encY = %ld" % (ency,)
            
        ans_len = 8
        ans_payload = chr(zy*128+zx*64+rzy*32+rzx*16+code_field)+chr((encx>>16)&255)+chr((encx>>8)&255)+chr(encx&255)+chr((ency>>16)&255)+chr((ency>>8)&255)+chr(ency&255)
        bAnswer = True
            
    else:
        print "bad controller id %d" % (dst_field, )
    
    # costruisci la risposta se richiesta
    if bAnswer:
        answer = chr(0x16)+chr(ans_len)+ans_payload
        # checksum, lunghezza inclusa nel calcolo
        ck = 0
        for c in answer[1:]:
            ck ^= ord(c)
        answer += chr(ck)
    else:
        answer = ""
        
    return (True, answer)

#k_de = 22700/360*2*pi   # 1 giro -> 360 gradi
#k_ar = 233496/24*2*pi   # 1 giro -> 24 ore
#k_pr = 203000/360*2*pi  # 1 giro -> 360 gradi
k_de = -22700/(2*pi)     # 1 giro -> 360 gradi
k_ar = 233496/(2*pi)     # 1 giro -> 24 ore
k_pr = 203000/(2*pi)     # 1 giro -> 360 gradi
ofs_de = -70*pi/180;     # offset per allineare col montaggio effettivo
ofs_ar = pi;
ofs_pr = 0;              # non abbiamo riferimenti visibili per la precessione

def simpla_motion():
    # simula movimenti planetario
    for motr in range(0, 3):
        if motr==0:
            mode=_opmode.decl
            spd=_opmode.decl_spd
            targ=_opmode.decl_target
            enc=_stat.decl_enc
        elif motr==1:
            mode=_opmode.ascr
            spd=_opmode.ascr_spd
            targ=_opmode.ascr_target
            enc=_stat.ascr_enc
        else:
            mode=_opmode.prec
            spd=_opmode.prec_spd
            targ=_opmode.prec_target
            enc=_stat.prec_enc
            
        bZ=False
        if mode==1:
            # ricerca zero
            # vai verso lo zero
            if enc>0:
                enc-=spd
                if enc<=0:
                    # siamo passati per lo zero
                    bZ=True
            elif enc<0:
                enc+=spd
                if enc>=0:
                    # siamo passati per lo zero
                    bZ=True
            else:
                bZ=True
            
            if bZ:
                # azzera un po' tutto
                mode=0
                spd=0
                targ=0
                enc=0
                
        elif mode==2:
            # goto target
            # applica semplice controllore P con saturazione
            err = (targ-enc)
            if err>500:
                delta=50
            elif err<-500:
                delta=-50
            elif err>-spd/5 and err<spd/5:
                delta=err
                mode=0
                spd=0
            else:
                delta=err/10
            if delta>spd:
                delta=spd
            elif delta<-spd:
                delta=-spd
            enc += delta
            bZ=(enc>-1 and enc<1)
            
        if motr==0:
            _opmode.decl=mode
            _opmode.decl_spd=spd
            _opmode.decl_target=targ
            _stat.decl_enc=enc
            _stat.decl_zero=bZ
        elif motr==1:
            _opmode.ascr=mode
            _opmode.ascr_spd=spd
            _opmode.ascr_target=targ
            _stat.ascr_enc=enc
            _stat.ascr_zero=bZ
        else:
            _opmode.prec=mode
            _opmode.prec_spd=spd
            _opmode.prec_target=targ
            _stat.prec_enc=enc
            _stat.prec_zero=bZ
            

light_curindex=-1
def luci():
    global light_curindex,  lidx
    # aggiorna le luci ciclicamente una alla volta (andiamo a 100 Hz!)
    light_curindex=(light_curindex+1)%len(num_light)
    if light_curindex==0:
        idx=lidx["SUPERNOVA"]
        value=_stat.supernova
        perc=value/4095.
    elif light_curindex==1:
        idx=lidx["NEBULA"]
        value=_stat.p_nebula
        perc=value/4095.
    elif light_curindex==2:
        idx=lidx["NUOVE_S"]
        value=_stat.nuove_s
        perc=value/4095.
    elif light_curindex==3:
        idx=lidx["STELLE"]
        value=_stat.stelle
        perc=value/800.
    elif light_curindex==4:
        idx=lidx["SOLE"]
        value=_stat.sole
        perc=value/4095.
    elif light_curindex==5:
        idx=lidx["LUNA"]
        value=_stat.luna
        perc=value/4095.
    elif light_curindex==6:
        idx=lidx["CERCHIO_O"]
        value=_stat.cerchio_orario
        perc=value/4095.
    elif light_curindex==7:
        idx=lidx["ALBA"]
        value=_stat.alba
        perc=value/255.
    elif light_curindex==8:
        idx=lidx["TRAMONTO"]
        value=_stat.tramonto
        perc=value/255.
    elif light_curindex==9:
        idx=lidx["GIORNO"]
        value=_stat.giorno
        perc=value/255.
    elif light_curindex==10:
        idx=lidx["PIANETI"]
        value=1 if _stat.pianeti else 0
        perc=value
    elif light_curindex==11:
        idx=lidx["PCARDINALI"]
        value=1 if _stat.p_cardinali else 0
        perc=value
    elif light_curindex==12:
        idx=lidx["SALA"]
        value=1 if _stat.luce_sala else 0
        perc=value
    elif light_curindex==13:
        idx=lidx["CERCHIO_P"]
        value=1 if _stat.cerchio_polare else 0
        perc=value
    else:
        idx=lidx["DECLIN"]
        value=1 if _stat.declinazione else 0
        perc=value

    num_light[idx].text=str(value)
    sph_light[idx].color=(0.3, perc, 0.3)


# inizio programma principale
# riempi la scena principale
scene.height=1024
scene.width=768
scene.center=(0, 0.5, 0)
scene.forward=(-0.895255956005113, -0.0060851552293899, -0.445510655454171) # trovato ruotando a mano + breakpoint
scene.title = "planetario di Marghera"

framew = frame()                # mondo
framed = frame(frame=framew)    # declinazione
framea = frame(frame=framed)    # ascensione retta
framepm = frame(frame=framea)   # montaggio precessione (intermedio fisso)
framep = frame(frame=framepm)    # precessione

# default frame (1,0,0): x=destra, y=alto, z=fuori dallo schermo
base = box(frame=framew, pos=(0, -0.1, 0), lenght=1, height=0.2, width=1,  color=color.gray(0.5))
arm1 = cylinder(frame=framew, pos=(0.45, 0, 0.45),  axis=(-0.1, 0.8, -0.35), radius=0.02,  color=color.gray(0.5))
arm2 = cylinder(frame=framew, pos=(0.45, 0, -0.45),  axis=(-0.1, 0.8, 0.35), radius=0.02,  color=color.gray(0.5))
arm3 = cylinder(frame=framew, pos=(-0.45, 0, 0.45),  axis=(0.1, 0.8, -0.35), radius=0.02,  color=color.gray(0.5))
arm4 = cylinder(frame=framew, pos=(-0.45, 0, -0.45),  axis=(0.1, 0.8, 0.35), radius=0.02,  color=color.gray(0.5))
supp1 = box(frame=framew, pos=(arm1.x+arm1.axis[0], arm1.axis[1], 0),  length=0.04, width=0.2, height=0.04, color=color.gray(0.5))
supp2 = box(frame=framew, pos=(arm3.x+arm3.axis[0], arm3.axis[1], 0),  length=0.04, width=0.2, height=0.04, color=color.gray(0.5))
framed.pos = supp1.pos  # centro del braccio destro
framed.axis = (1, 0, 0) # rotazioni da applicare al piano yz
framed.up = (0, 1, 0)
arm_decl = cylinder(frame=framed, pos=(0, 0, 0),  axis=(supp2.x-supp1.x, 0, 0), radius=0.02,  color=color.gray(0.5))
#arm5 = cylinder(frame=framew, pos=supp1.pos,  axis=supp2.pos-supp1.pos, radius=0.02,  color=color.gray(0.5))
framea.pos = arm_decl.pos+arm_decl.axis/2   # centro di arm_decl
framea.axis = (0, 1, 0) # rotazioni da applicare al piano xz
framea.up=(-1, 0, 0)
support_ascretta = cylinder(frame=framea,  pos=(0.08, 0, 0), axis=(-0.12, 0, 0), radius=0.1, color=color.gray(0.5))
arm_ascretta = cylinder(frame=framea,  pos=(-0.04, 0, 0), axis=(-0.68, 0, 0.2), radius=0.025, color=color.gray(0.5))
framepm.pos = support_ascretta.pos # cima di support_ascretta
framepm.axis = (cos(radians(23)), 0, -sin(radians(23)))
framep.pos = (0, 0, 0.05)  # ancora cima di arm_ascretta, ma ruotati e fuori centro
framep.axis= (1, 0, 0)  # rotazioni da applicare al piano yz
framep.up = (0, 1, 0)
arm_prec = cylinder(frame=framep, pos=(-0.05, 0, 0), axis=(0.4, 0, 0), radius=0.04, color=color.gray(0.5))
starball = sphere(frame=framep, pos=arm_prec.pos+arm_prec.axis, radius=0.3, color=(0.6,0.6,0.3), material=materials.BlueMarble)

# riempi la scena con le luci
lname=["SOLE", "LUNA", "STELLE", "TRAMONTO", "ALBA", "GIORNO", "PIANETI", "SUPERNOVA", "NEBULA", 
    "NUOVE_S", "CERCHIO_O", "PCARDINALI", "SALA", "CERCHIO_P", "DECLIN"]
lidx={lname[i]:i for i in range(0,len(lname))}
scene2 = display(title="luci planetario", width=600, height=350, x=768, y=0, range=(3, 3, 2))
lbl_light=[]
sph_light=[]
num_light=[]
for row in range(0, 3):
    for col in range(0, 5):
        idx=row*5+col
        sph_light.append(sphere(pos=(col-2, 1-row, 0), radius=0.4, color=color.green))
        lbl_light.append(label(pos=sph_light[idx].pos-(0, 0.3, 0), text=lname[idx], line=False))
        num_light.append(label(pos=sph_light[idx].pos-(0, 0, 0), text="0", line=False, box=False, opacity=0))

# apri seriale per parlare col programma di gestione
ser = serial.Serial(port='COM1', baudrate=4800, timeout=0)

# loop di aggiornamento
while True:
    rate(100) # controllori girano a 32.768 ms (circa 30.5 Hz)
    while True:
        # stranamente, senza il precedente while e con un ser.read(100), o anche con lo while e ad es. ser.read(10),
        #   perde dei caratteri in ricezione: ne derivano occasionali "checksum err"+"bad len field 0" qui,
        #   occasionali timeout motori lato chiamante. Con ser.read(1) va bene (ovviamente consuma molta CPU),
        #   ma va bene anche con 5. Che c'entri l'adattatore 232-USB?
        data = ser.read(5)
        if len(data):
            if dbg_verbose:
                for c in data:
                    print "%02X "  % (ord(c), ), 
            response=simpla_msg(data)
            if response[0]:
                if len(response[1]):
                    # rispondi
                    ser.write(response[1])
                    if dbg_verbose:
                        print "--> ", 
                        for c in response[1]:
                            print "%02X "  % (ord(c), ), 
                        print
                else:
                    if dbg_verbose:
                        print "--> no answer"
        else:
            break
                    
    # ciclo di controllo (doppio quando eseguito anche in simpla_msg...
    simpla_motion()

    ang_de = _stat.decl_enc/k_de+ofs_de
    ang_ar = _stat.ascr_enc/k_ar+ofs_ar
    ang_pr =  _stat.prec_enc/k_pr+ofs_pr
    framed.up=(0, cos(ang_de), sin(ang_de))
    framea.up=(-cos(ang_ar), 0,  sin(ang_ar))
    framep.up=(0, cos(ang_pr), sin(ang_pr))
    
    luci()
