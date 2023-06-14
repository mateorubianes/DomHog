#include "mbed.h"
#include "TextLCD.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Ticker tick;

TextLCD lcd(PTC6, PTC5, PTC4, PTC3, PTC0, PTC7, TextLCD::LCD20x4); // rs, e, d4-d7

#define min_disp 1
#define max_disp 2
#define tipos_disp 3
#define conf_menu 3
#define renglones_LCD 4 // LCD20x4 = 4   o   LCD 16x2 = 2

#define BAUD_RATE   9600
#define INICIO      '$'
#define FIN         '#'
#define REENVIO     '='
#define LARGO_DATOS 5
// Trama $C1234V#

// Comandos
#define COMANDO_menu    'M'
#define COMANDO_disp    'D'

//#define VERIFICACION_RECEPCION

enum{
    REPOSO = 0,
    COMANDO,
    DATOS,
    VERIFICACION,
    REENVIAR
};
/*
Baud rate: velocidad de transferencia de datos, por defecto 9600, máximo 115200
Debo tener el mismo baudrate en el transmisor y el receptor
Debemos seleccionar una de las 3 uarts del KL25:
UART0: TX PTA2,  RX PTA1, es la UART del USB
UART1: TX PTE0,  RX PTE1
UART2: TX PTE22, RX PTE23
*/
//En mbed, disponemos de la clase Serial para implementar la UART
//       PIN TX  PIN RX   
Serial pc(USBTX, USBRX, BAUD_RATE);
// BAUD_RATE: 4800, 9600, 14400, ..., 115200

/* Funciones que utlizaremos de la clase Serial:
pc.putc(char caracter) -> Enviar un caracter
pc.getc() -> Retornar un caracter
pc.readable() -> Avisa cuando hay algo disponible para leer
*/


// Maquina de estados para la recepcion y transmision (MRTX)
char comando = 0; // Comando recibido por serie
char datos[LARGO_DATOS]; // Vector donde recibo los datos del serie
char estado_RX = REPOSO; // Estado maquina de estados
char envio_datos[LARGO_DATOS];

void maquina_recepcion(void);
void envio_trama(char * datos);
void funcion_comandos(char comando, char * datos);

// Maquina de estados para el display (MLCD)
void maquina_LCD();
char menu_disp = 0;
    enum{
        luz = 1,
        enchufe,
        persiana,
        };
char sub_menu[tipos_disp+3][16] =   {"CONFIGURACION",
                                     "Temp. Deseada",
                                     "Control Total",
                                     "Luces",
                                     "Enchufes",
                                     "Persianas"};
                                 
char nombre_disp[tipos_disp][max_disp][16] =    {"Luz ON/OFF",
                                                 "Luz DIMMER",
                                                 "Calor/Frio",
                                                 "",
                                                 "Habitacion",
                                                 ""};               
     
int est_disp[tipos_disp][max_disp] =    {1,9,
                                         0,
                                         0};

int est_disp_copia[tipos_disp][max_disp] =    {1,9,
                                               0,
                                               0};
                                         
int est_disp_enviar[tipos_disp][max_disp] =    {1,9,
                                                0,
                                                0};

void envio_data();
                                  
int id_luz[max_disp] = {1,2};

int temp[2] = {2,4};
int temp_deseada[2] = {2,4};
int temp_copia[2] = {2,4};
char temp_enviar[4] = "D24";
int pres[3] = {0,0,9};

    enum{                   
        carga = 0,
        menu,
        accion,
        };
        
    enum{
        tdeseada = 3,
        total,
        disp1,
        disp2,
        disp3,
        };
        
static char estado = carga;

//Encoder
        
int contador_encoder = 0;
char modificador = 0;
DigitalIn clk(PTB2);
DigitalIn dt(PTB3);

char estado_anterior, estado_actual, detector = 0;
char sumador = 0;

void maquina_encoder();

//Maquina de estados para los pulsadores (MPULS)

DigitalIn P0(PTE30);
DigitalIn P1(PTE29);
DigitalIn P2(PTE23);
DigitalIn P3(PTC2);
char flag_pul0 = 0;
char flag_pul1 = 0;
char flag_pul2 = 0;
char flag_pul3 = 0;
    enum{ 
        out_on = 0,
        out_off
    };

char Pul0();
char Pul1();
char Pul2();
char Pul3();
int S0, S1, S2, S3;



char flag_LCD = 0;
int time_LCD = 300;  // puede ser cualquier valor superior al maximo seteado para el cls.
char flag_retorno = 0;
int retorno = 0;
char flag_carga = 0;
int valor = 0;

void interuption()
{
    //MLCD
    flag_LCD = 1;
    flag_retorno = 1;
    flag_carga = 1;
    //MPULS
    flag_pul0 = 1;
    flag_pul1 = 1;
    flag_pul2 = 1;
    flag_pul3 = 1;
}   

int main() {
    
    tick.attach(&interuption, 0.001); // setup ticker to call flip every 0.1 seconds
    P0.mode(PullNone);
    P1.mode(PullNone);
    P2.mode(PullNone);
    P3.mode(PullNone);
    clk.mode(PullNone);
    dt.mode(PullNone);
    //
    estado_anterior = dt;
    //
    
    while(1) {
        //
            maquina_encoder();
        //
            maquina_recepcion();
            maquina_LCD(); 
            envio_data();  
        
        if(estado != carga)
        {
            if(modificador != 1)
            {
                /* Al detectarse un flanco high del pulsador 0
                Se cambia la pantalla del menu al submenu de luces
                */
                if(Pul0() == 1)
                {
                    estado = disp1;
                    menu_disp = luz;
                    retorno = 0;
                    contador_encoder = 0;
                    time_LCD = 900;
                }
                
                /* Al detectarse un flanco high del pulsador 1
                   Se cambia la pantalla del menu al submenu de enchufes
                */
                if(Pul1() == 1)
                {
                    estado = disp2;
                    menu_disp = enchufe;
                    retorno = 0;
                    contador_encoder = 0;
                    time_LCD = 900;
                }
                
                /* Al detectarse un flanco high del pulsador 2
                   Se cambia la pantalla del menu al submenu de persianas
                */
                if(Pul2() == 1)     
                {
                    estado = disp3;
                    menu_disp = persiana;
                    retorno = 0;
                    contador_encoder = 0;
                    time_LCD = 900;
                }
            }
            
            /* Al detecarse un flanco high del pulsador 3
               Se detiene la pantalla para permitir al usuario cambiar parametros del dispositivo seleccionado
               Ademas se produce un cambio de funcionamiento del encoder para funcionar como modificador de parametros 
               (No moviendo el submenu interactivo)
            */
            if(Pul3() == 3)     
            {   
                if(estado > menu)
                    retorno = 0;
                if(estado > total)
                {
                    if(estado != disp2)
                    {   
                        modificador++;
                        time_LCD = 900;
                    }   
                }
                
                switch(estado)
                {
                    case accion:
                        estado = contador_encoder+3;
                        contador_encoder = 1;
                        retorno = 0;
                        
                        if(estado == total)
                        {
                            for(int l = 0; l < max_disp; l++)
                            {
                                for(int i = 0; i < tipos_disp; i++)
                                {
                                    if(i != 1)
                                    {
                                        sumador = sumador + est_disp[i][l];
                                    }
                                }
                            }
                        }
                        time_LCD = 900;
                    break;
                    
                    case tdeseada:
                        temp_deseada[0] = temp_copia[0];
                        temp_deseada[1] = temp_copia[1];
                         time_LCD = 900;
                        retorno = 10000;
                        estado = menu;
                    break;
                    
                    case total:
                        for(int l = 0; l < max_disp; l++)
                            {
                                for(int i = 0; i < tipos_disp; i++)
                                {
                                    // Luces
                                    if(i == 0 && id_luz[l]== 1)
                                    {
                                        if(sumador == 0)
                                            est_disp[i][l] = 1;
                                        else
                                            est_disp[i][l] = 0;
                                    }
                                    
                                    if(i == 0 && id_luz[l]== 2)
                                    {
                                        if(sumador == 0)
                                            est_disp[i][l] = 10;
                                        else
                                            est_disp[i][l] = 0;
                                    }
                                    //
                                    
                                    // Enchufe
                                    if(i == 1)
                                    {
                                    }
                                    //
                                    
                                    // Persiana
                                    if(i == 2)
                                    {
                                        if(sumador == 0)
                                            est_disp[i][l] = 2;
                                        else
                                            est_disp[i][l] = 0;
                                    } 
                                    //
                                }
                            }
                        sumador = 0;              
                        retorno = 9500;
                        time_LCD = 900;         
                    break;
                    
                    case disp2:
                        modificador = 2;
                    break;
                }
                
                if(modificador > 1)
                {
                    modificador = 0;
                    time_LCD = 900;
                    estado = menu;
                    
                    retorno = 0;
                }
            }
        }
        else
            menu_disp = carga;  
    }
}

void maquina_LCD()
{
    static char cuenta = 0;
    char ubicador = 0;
    char i = 0;
    
    // Configuro el tiempo de clear del LCD
    if(flag_LCD == 1)
    {
        time_LCD++;
        flag_LCD = 0;
    }
    
    switch(estado)
    {
        // Pantalla de carga/conexion a la red
        case carga:
            if(flag_carga == 1)
            {
                valor++;
                flag_carga = 0;
                    
                if(valor >= 10000)
                {
                    estado = menu;
                    valor = 0;
                }
            }
            // Configuro el tiempo de clear del LCD
            if(time_LCD >= 1000)
            {
                if(renglones_LCD >= 4)
                {
                    lcd.cls();
                    lcd.locate(1,0);
                    lcd.printf("Dispositivo DOMHOG");
                    cuenta++;
                    
                    switch(cuenta)
                    {
                        case 1:
                            lcd.locate(4,2);
                            lcd.printf("LOADING .");
                        break;
                        
                        case 2:
                            lcd.locate(4,2);
                            lcd.printf("LOADING ..");
                        break;
                        
                        case 3:
                            lcd.locate(4,2);
                            lcd.printf("LOADING ...");
                        break;
                        
                        default:
                            cuenta = 1;
                            lcd.locate(4,2);
                            lcd.printf("LOADING .");
                        break;
                    }
                }
                else
                {
                    lcd.cls();
                    lcd.locate(0,0);
                    lcd.printf("|--  DOMHOG  --|");
                    cuenta++;
                    
                    switch(cuenta)
                    {
                        case 1:
                            lcd.locate(2,1);
                            lcd.printf("LOADING .");
                        break;
                        
                        case 2:
                            lcd.locate(2,1);
                            lcd.printf("LOADING ..");
                        break;
                        
                        case 3:
                            lcd.locate(2,1);
                            lcd.printf("LOADING ...");
                        break;
                        
                        default:
                            cuenta = 1;
                            lcd.locate(2,1);
                            lcd.printf("LOADING .");
                        break;
                    }
                }
                time_LCD = 0;               
            }
        break;
        
        // Menu Principal
        case menu:
            // Configuro el tiempo de clear del LCD
            if(time_LCD >= 1000)
            {             
                if(renglones_LCD >= 4)
                {
                    lcd.cls();
                    lcd.locate(0,0);
                    lcd.printf("Temperatura=  ");
                    if(temp[0] >= 10 && temp[1] >= 0)
                        lcd.printf("-");
                    else
                        lcd.printf(" ");
                    if(temp[0]%10 != 0)
                        lcd.putc(temp[0]%10+48);
                    lcd.putc(temp[1]+48);
                    lcd.printf("C");
                    
                    
                    lcd.locate(0,1);
                    lcd.printf("Temp. Deseada= ");
                    if(temp[0]%10 != 0)
                        lcd.putc(temp_deseada[0]%10+48);
                    lcd.putc(temp_deseada[1]+48);
                    lcd.printf("C");
                    
                    
                    lcd.locate(0,3);
                    lcd.printf("Presion=    ");
                    if(pres[0] != 0)
                    {
                        lcd.putc(pres[0]+48);
                    }
                    else
                        lcd.printf(" ");
                    if(pres[0] != 0 && pres[1] == 0)
                        lcd.putc(48);
                    else
                        if(pres[1] != 0)
                            lcd.putc(pres[1]+48);
                        else
                            lcd.printf(" ");
                    lcd.putc(pres[2]+48);
                    lcd.printf("hPa");
                }
                else
                {
                    lcd.cls();
                    lcd.locate(0,0);
                    lcd.printf("T=");
                    if(temp[0] >= 10 && temp[1] >= 0)
                        lcd.printf("-");
                    if(temp[0]%10 != 0)
                        lcd.putc(temp[0]%10+48);
                    lcd.putc(temp[1]+48);
                    lcd.printf("C");
                    
                    lcd.locate(8,0);
                    lcd.printf("H=");
                    if(pres[0] != 0)
                    {
                        lcd.putc(pres[0]+48);
                    }
                    else
                        lcd.printf(" ");
                    if(pres[0] != 0 && pres[1] == 0)
                        lcd.putc(48);
                    else
                        if(pres[1] != 0)
                            lcd.putc(pres[1]+48);
                        else
                            lcd.printf(" ");
                    lcd.putc(pres[2]+48);
                    lcd.putc(37);
                    
                    lcd.locate(0,1);
                    lcd.printf("T deseada= ");
                    if(temp[0]%10 != 0)
                        lcd.putc(temp_deseada[0]%10+48);
                    lcd.putc(temp_deseada[1]+48);
                    lcd.printf("C");
                }
                
                time_LCD = 0;
                contador_encoder = 0;
            }
        break;
        
        case accion: 
            if(time_LCD >= 1000)
            {
                    lcd.cls();
                                       
                    for(char renglones = 0; renglones < renglones_LCD; renglones++)
                    {
                        if((contador_encoder + renglones) == tipos_disp+3)
                            break;
                        
                        lcd.locate(0,renglones);
                        if(contador_encoder == 0 && renglones == 0)
                        {}
                        else
                        {
                            if(renglones == 1)
                                lcd.printf("> ");
                            else
                                lcd.printf("  ");
                        }
                        
                        while(sub_menu[contador_encoder + renglones][i] != '\0')
                        {
                            lcd.putc(sub_menu[contador_encoder + renglones][i]);
                            i++;
                        }
                        i = 0;
                    }
                        i = 0;
                time_LCD = 0;
            }
        break;
        
        case tdeseada:
            if(time_LCD >= 1000)
            {
                if(renglones_LCD >= 4)
                {
                    lcd.cls();
                    lcd.locate(0,1);
                    lcd.printf("Temp. Deseada= ");
                    
                    if(temp_copia[0]%10 != 0)
                        lcd.putc(temp_copia[0]%10+48);
                    lcd.putc(temp_copia[1]+48);
                    lcd.printf("C");
                }
                else
                {
                    lcd.cls();
                    lcd.locate(0,0);
                    lcd.printf("Temperatura");
                    lcd.locate(0,1);
                    lcd.printf("Deseada= ");
                    
                    if(temp_copia[0]%10 != 0)
                        lcd.putc(temp_copia[0]%10+48);
                    lcd.putc(temp_copia[1]+48);
                    lcd.printf("C");
                }             
                time_LCD = 0;
            }
        
        case total:
            if(time_LCD >= 1000)
            {
                if(renglones_LCD >= 4)
                {
                    lcd.cls();
                    lcd.locate(0,1);
    
                    lcd.printf("Estado de los");
                    lcd.locate(0,2);
                    lcd.printf("Dispositivos:");
                    
                    if(sumador != 0)
                        lcd.printf("  ON");
                    else
                        lcd.printf(" OFF");
                }
                else
                {
                    lcd.cls();
                    lcd.locate(0,0);
                    lcd.printf("Estado de los");
                    lcd.locate(0,1);
                    lcd.printf("Dispositivos:");
                    
                    if(sumador != 0)
                        lcd.printf("  ON");
                    else
                        lcd.printf(" OFF");
                }                
                time_LCD = 0;
            }
        break;
    }
    
    if(estado > total)
    {
        if(time_LCD >= 1000)
            {
                if(contador_encoder == 0)
                {
                    lcd.cls();
                    ubicador = 7-(strlen(sub_menu[estado-2])/2);
                    lcd.locate(ubicador,0);
                    while(sub_menu[estado-2][i] != '\0')
                    {
                        lcd.putc(sub_menu[estado-2][i]);
                        i++;
                    }
                    i = 0;
                    
                }
                else
                {
                    lcd.cls();
                    if(nombre_disp[estado-5][contador_encoder-1][0] == '\0')
                        contador_encoder--;
                    lcd.locate(0,0);
                    //lcd.putc(strlen(nombre_disp[estado-5][contador_encoder-1])+48);
                    while(nombre_disp[estado-5][contador_encoder-1][i] != '\0')
                    {
                        lcd.putc(nombre_disp[estado-5][contador_encoder-1][i]);
                        i++;
                    }
                    i = 0;
                    lcd.printf(":");
                    
                    if(renglones_LCD >= 4)
                        lcd.locate(0,2);
                    else
                        lcd.locate(0,1);
                        
                    switch(estado)
                    {
                        case disp1:
                            lcd.printf("Nvl de Luz: ");
                            switch(id_luz[contador_encoder-1])
                            {
                                case 1:
                                    if(est_disp[estado-5][contador_encoder-1] == 0)
                                        lcd.printf("OFF");
                                    if(est_disp[estado-5][contador_encoder-1] == 1)
                                        lcd.printf("ON");
                                break;
                                
                                case 2:
                                    if(est_disp[estado-5][contador_encoder-1] == 0)
                                    {
                                        lcd.printf("OFF");
                                    }
                                    else
                                    {
                                        if(est_disp[estado-5][contador_encoder-1] == 10)
                                        {
                                            lcd.printf("Full");
                                        }
                                        else
                                        {
                                            lcd.putc(est_disp[estado-5][contador_encoder-1]+48);
                                        }
                                    }
                                break;
                            }
                        break;
                        
                        case disp2:
                            lcd.printf("Estado: ");
                            if(est_disp[estado-5][contador_encoder-1] == 0)
                            {
                                lcd.printf("OFF");
                            }
                            else
                            {
                                lcd.printf("ON");
                            }
                        break;
                        
                        case disp3:
                            lcd.printf("Nvl Altura: ");
                            if(est_disp[estado-5][contador_encoder-1] == 0)
                            {
                                lcd.printf("Down");
                            }
                            else
                            {
                                if(est_disp[estado-5][contador_encoder-1] == 2)
                                {
                                    lcd.printf("Up");
                                }
                                else
                                {
                                    lcd.printf("Half");
                                }
                            }
                        break;        
                    }
                }  
                time_LCD = 0;        
            }
    }
        // Tiempo para retornar del submenu al menu principal
        if(estado > menu)
        {
            if(modificador != 1)
            {
                if(flag_retorno == 1)
                {
                    retorno++;
                    flag_retorno = 0;
                    if(retorno >= 5000)
                    {
                        estado = menu;
                        retorno = 0;
                        contador_encoder = 0;
                        menu_disp = carga;
                    }
                    
                    if(retorno >= 3000 && estado == disp2)
                    {
                        estado = menu;
                        retorno = 0;
                        contador_encoder = 0;
                        menu_disp = carga;
                    }
                }
            }
        }
}

// Funcion de verificacion mediante XOR de todos los datos ingresados
char checksum_verif(char * data, int largo) // Paso el vector de datos
{
    char chksum = 0;
    // Hago el XOR de cada uno de los bytes de la trama
    for(int i = 0; i < largo; i++)
        chksum = chksum ^ data[i]; // XOR Byte a byte
    
    return chksum; // Devuelvo el byte de verificacion hecho localmente
}

/*
Defino un protocolo arbitrario
$ab...bc#
$: caracter de inicio
a: comando (1 byte), vale cualquier caracter
b...b: datos (largo indefinido)
c: verificacion
#: caracter de fin
*/
void maquina_recepcion(void)
{
    static int indice = 0; // Indice para vector de datos de comando
    
    char caracter_recibido = 0; // Caracter donde guardamos el dato recibido
    char verificacion = 0; // Verificacion local de los datos
    char verificacion_recibida = 0; // Verificacion recibida por serial
    
    // Me fijo si hay algo disponible para recibir
    if(pc.readable()){
        caracter_recibido = pc.getc(); // Leo un caracter
    }
    else
      return; // Si no hay caracter disponible me voy de la funcion, no evaluo el switch
    
    switch(estado_RX)
    {
        case REPOSO:
            // Espero a que llegue un caracter de inicio
            if(caracter_recibido == INICIO)
                estado_RX = COMANDO;
            if(caracter_recibido == REENVIO)
                estado_RX = REENVIAR;
            break;
        case COMANDO: // El proximo byte que llegue despues del inicio es el comando
            // Recibo el comando e inmediatamente paso a recibir los datos,
            // luego se evaluará si el comando es válido
            comando = caracter_recibido;
            estado_RX = DATOS;
            break;
        case DATOS:
            // Espero a que llegue el caracter de fin para dejar de recibir datos
            if(caracter_recibido == FIN)
            {
                indice--; // Por el ultimo aumento al igualar en el vector
                // Evaluo si mi trama llego ok, hago la verificacion
                // Utilizo un define para evaluar si hago o no la verificacion
                #ifdef VERIFICACION_RECEPCION
                    verificacion = checksum_verif(datos, indice);
                
                #else
                    verificacion = datos[indice]; // No controlo verificacion
                #endif
                // datos[indice] sera el byte de verificacion recibido
                verificacion_recibida = datos[indice];
                if(verificacion != verificacion_recibida)
                {
                    // Si falla la verificacion no guardo los datos
                    comando = 0;
                    // Limpio el vector
                    //    Vector    Valor   Largo
                    memset(datos,     0,    indice);
                }
                else
                {
                    // Si la verificacion es correcta ejecuto el comando
                    funcion_comandos(comando, datos);
                }
                indice = 0; // Reseteo el indice cuando termino de recibir
                estado_RX = REPOSO;
                break;
            }
            // Recibo datos y aumento el indice
            datos[indice++] = caracter_recibido;
            break;
        case REENVIAR:
            envio_trama("=");
            estado_RX = REPOSO;
        break;
    }
}

void funcion_menu(char * datos)
{
    // Utilizo los datos que recibo para dividir la informaciòn recibida en la variables correspondientes
    // Resto '0' ya que recibo en ASCII
    temp[0] = (datos[0]-36)/10;
    temp[1] = (datos[0]-36)%10;
    pres[0] = (datos[1]-36)/100;
    pres[1] = ((datos[1]-36)/10)%10;
    pres[2] = (datos[1]-36)%10;
    temp_deseada[0]  = (datos[2]-36)/10;
    temp_deseada[1]  = (datos[2]-36)%10;
    temp_copia[0]    = (datos[2]-36)/10;
    temp_copia[1]    = (datos[2]-36)%10;
}

void funcion_disp(char * datos)
{
    // Utilizo los datos que recibo para dividir la informaciòn recibida en la variables correspondientes
    est_disp[0][0] = datos[0] - 48;
    est_disp[0][1] = datos[1] - 48;
    est_disp[1][0] = datos[2] - 48;
    est_disp[2][0] = datos[3] - 48;
}

void funcion_comandos(char comando, char * datos)
{
    // En funcion de la letra que recibo ejecuto distintas funciones
    switch(comando)
    {
        case COMANDO_menu:
            funcion_menu(datos);
            break;
        case COMANDO_disp:
            funcion_disp(datos);
            break;
    }
}
// $ab...bc#
void envio_trama(char * data)
{
    char buffer[50];
    char verificacion = 0;
    char index = 0;
    /*            
    sprintf nos "imprime" al buffer como si fuera un printf,
    solo que en vez de imprimir a la pantalla, escribe el string
    que le indiquemos. El valor que retorna será la cantidad de caracteres
    que logro escribir.        $ a Datos*/
    index += sprintf(buffer, "%c%s",  INICIO,  
                                        data);
    // Hago la verificacion para agregarla a la trama
    verificacion = checksum_verif(buffer, strlen(buffer));
    /* Sumo index para no pisar lo ya escrito sobre buffer y comenzar a
    escribir después de la ultima posición escrita anteriormente */
    sprintf(buffer + index, "%c%c", verificacion, 
                                    FIN);
    // Envio trama con la funcion putc y el largo del buffer formado
    for(int i = 0; i < strlen(buffer); i++)
        pc.putc(buffer[i]);
}

//Funcion P0
char Pul0()
{
    static char value = out_on;
    static char time_pul0 = 0;
    char flanco;
    
    if(flag_pul0 == 1)
    {
        time_pul0 ++;
        flag_pul0 = 0;
    }
      
    switch(value)
    {
        case out_on:
            if(P0 == 0 && time_pul0 < 30)
                {
                    flanco = 0;
                    //led1=1;
                    value = out_off;
                }
            else
                {   
                    if(P0 == 1)
                    {
                        //led1=0;
                        time_pul0 = 0;
                        flanco = 2;
                    }
                }
            break;
        case out_off:
            if(P0 == 1 && time_pul0 > 30)
                {
                    flanco = 1;
                    //led1=0;
                    value = out_on;
                }
            else
                {
                    if(P0 == 0)
                    {
                        //led1=1;
                        time_pul0 = 0;
                        flanco = 0;
                    }
                }
            break;
        default:
            value = out_on;
    }
    return flanco;
}


// Funcion P1
char Pul1()
{
    static char value = out_on;
    static char time_pul1 = 0;
    char flanco;
    
    if(flag_pul1 == 1)
    {
        time_pul1 ++;
        flag_pul1 = 0;
    }
      
    switch(value)
    {
        case out_on:
            if(P1 == 0 && time_pul1 < 30)
                {
                    flanco = 0;
                    //led1=1;
                    value = out_off;
                }
            else
                {   
                    if(P1 == 1)
                    {
                        //led1=0;
                        time_pul1 = 0;
                        flanco = 2;
                    }
                }
            break;
        case out_off:
            if(P1 == 1 && time_pul1 > 30)
                {
                    flanco = 1;
                    //led1=0;
                    value = out_on;
                }
            else
                {
                    if(P1 == 0)
                    {
                        //led1=1;
                        time_pul1 = 0;
                        flanco = 0;
                    }
                }
            break;
        default:
            value = out_on;
    }
    return flanco;
}


// Funcion P2
char Pul2()
{
    static char value = out_on;
    static char time_pul2 = 0;
    char flanco;
    
    if(flag_pul2 == 1)
    {
        time_pul2 ++;
        flag_pul2 = 0;
    }
      
    switch(value)
    {
        case out_on:
            if(P2 == 0 && time_pul2 < 30)
                {
                    flanco = 0;
                    //led1=1;
                    value = out_off;
                }
            else
                {   
                    if(P2 == 1)
                    {
                        //led1=0;
                        time_pul2 = 0;
                        flanco = 2;
                    }
                }
            break;
        case out_off:
            if(P2 == 1 && time_pul2 > 30)
                {
                    flanco = 1;
                    //led1=0;
                    value = out_on;
                }
            else
                {
                    if(P2 == 0)
                    {
                        //led1=1;
                        time_pul2 = 0;
                        flanco = 0;
                    }
                }
            break;
        default:
            value = out_on;
    }
    return flanco;
}

// Funcion P3
char Pul3()
{
    static char value = out_on;
    static char time_pul3 = 0;
    char flanco;
    
    if(flag_pul3 == 1)
    {
        time_pul3 ++;
        flag_pul3 = 0;
    }
      
    switch(value)
    {
        case out_on:
            if(P3 == 0 && time_pul3 < 30)
                {
                    flanco = 3;
                    //led1=1;
                    value = out_off;
                }
            else
                {   
                    if(P3 == 1)
                    {
                        //led1=0;
                        time_pul3 = 0;
                        flanco = 2;
                    }
                }
            break;
        case out_off:
            if(P3 == 1 && time_pul3 > 30)
                {
                    flanco = 1;
                    //led1=0;
                    value = out_on;
                }
            else
                {
                    if(P3 == 0)
                    {
                        //led1=1;
                        time_pul3 = 0;
                        flanco = 0;
                    }
                }
            break;
        default:
            value = out_on;
    }
    return flanco;
}

void maquina_encoder()
{    
    /* Funcion de deteccion de sentido de giro del encoder y accionar dependiendo su movimiento.
       Se compara el estado atual del encoder con el del estado anterior, si estos son diferentes
       se detecta un movimiento.
       Se procede a identificar sentido de giro por medio de analizar cual de las señales de clk o dt
       se antepone entre ellas y a que estado cambiaron.
    */
    estado_actual = dt;
        if(estado_actual != estado_anterior)
        {
                if(clk != estado_actual)
                {
                    detector++;
                    // Si se detecta que el switch del encoder se presiono
                    // Se ejecuta el cambio de estado del dispositivo seleccionado
                    if(modificador == 1)
                    {
                        if(detector == 2)
                        {
                            est_disp[estado-5][contador_encoder-1]--;
                            retorno = 0;
                            time_LCD = 1000;
                            detector = 0;
                        }                
                    }
                    // En el caso que no se detecte un cambio en el switch
                    // Se ejecuta la posivilidad de cambio de submenu
                    else
                    {   
                        if(detector == 2)
                        {
                            switch(estado)
                            {
                                case carga:
                                break;
                                
                                case menu:
                                    estado = accion;
                                    retorno = 0;
                                break;
                                
                                case accion:
                                    contador_encoder--;
                                    retorno = 0;
                                break;
                                
                                case tdeseada:
                                    temp_copia[1]--;
                                    retorno = 0;
                                break;
                                
                                case disp1:
                                    contador_encoder--;
                                    retorno = 0;
                                break;
                                
                                case disp2:
                                    contador_encoder--;
                                    retorno = 0;
                                break;
                                
                                case disp3:
                                    contador_encoder--;
                                    retorno = 0;
                                break;
                            }
                            detector = 0;
                            time_LCD = 1000;
                        }
                    }
                }
                else
                {
                    detector++;
                    // Si se detecta que el switch del encoder se presiono
                    // Se ejecuta el cambio de estado del dispositivo seleccionado
                    if(modificador == 1)
                    {
                        if(detector == 2)
                        {
                            est_disp[estado-5][contador_encoder-1]++;
                            retorno = 0;
                            time_LCD = 1000;
                            detector = 0;
                        }
                    }
                    // En el caso que no se detecte un cambio en el switch
                    // Se ejecuta la posivilidad de cambio de submenu
                    else
                    {
                        if(detector == 2)
                        {
                            switch(estado)
                            {
                                case carga:
                                break;
                                
                                case menu:
                                    estado = accion;
                                    retorno = 0;
                                break;
                                
                                case accion:
                                    contador_encoder++;
                                    retorno = 0;
                                break;
                                
                                case tdeseada:
                                    temp_copia[1]++;
                                    retorno = 0;
                                break;
                                                         
                                case disp1:
                                    contador_encoder++;
                                    retorno = 0;
                                break;
                                
                                case disp2:
                                    contador_encoder++;
                                    retorno = 0;
                                break;
                                
                                case disp3:
                                    contador_encoder++;
                                    retorno = 0;
                                break;
                                
                            }
                            time_LCD = 1000;
                            detector = 0;
                        }
                    }
                }
                if(modificador == 1)
                {
                    // Limimites de extremos de modulos_luz
                    switch(estado)
                    {                                
                        case disp1:
                            if(id_luz[contador_encoder-1] == 1)
                            {
                                if(est_disp[estado-5][contador_encoder-1] > 1)
                                {
                                    est_disp[estado-5][contador_encoder-1] = 1;
                                }
                                if(est_disp[estado-5][contador_encoder-1] < 0)
                                {
                                    est_disp[estado-5][contador_encoder-1] = 0;
                                }
                            }
                            
                            if(id_luz[contador_encoder-1] == 2)
                            {
                                if(est_disp[estado-5][contador_encoder-1] > 10)
                                {
                                    est_disp[estado-5][contador_encoder-1] = 10;
                                }
                                if(est_disp[estado-5][contador_encoder-1] < 0)
                                {
                                    est_disp[estado-5][contador_encoder-1] = 0;
                                }
                            }
                        break;
                        
                        // Limimites de extremos de modulos_enchufe
                        case disp2:
                            if(est_disp[estado-5][contador_encoder-1] > 1)
                            {
                                est_disp[estado-5][contador_encoder-1] = 1;
                            }
                            if(est_disp[estado-5][contador_encoder-1] < 0)
                            {
                                est_disp[estado-5][contador_encoder-1] = 0;
                            }
                        break;
                        
                        // Limimites de extremos de modulos_persiana
                        case disp3:
                            if(est_disp[estado-5][contador_encoder-1] > 2)
                            {
                                est_disp[estado-5][contador_encoder-1] = 2;
                            }
                            if(est_disp[estado-5][contador_encoder-1] < 0)
                            {
                                est_disp[estado-5][contador_encoder-1] = 0;
                            }
                        break;
                    }
                }
                else
                {
                    // Limimites de extremos del submenu
                    if(estado == accion)
                    {
                        if(contador_encoder < 0)
                            contador_encoder = 0;
                        if(contador_encoder > (tipos_disp+1))
                            contador_encoder = (tipos_disp+1);
                    }
                    
                    if(estado > total)
                    {
                        if(contador_encoder < 1)
                            contador_encoder = 1;
                        if(contador_encoder > max_disp)
                            contador_encoder = max_disp;
                    }
                    
                    // Limimites de extremos en la temperatura
                    if(temp_copia[1] > 9)
                    {
                        temp_copia[0]++;
                        temp_copia[1] = 0;
                    }
                    if(temp_copia[1] < 0)
                    {   
                        temp_copia[0]--;
                        temp_copia[1] = 9;
                    }
                    
                    if(temp_copia[0] >= 3 && temp_copia[1] >= 0)
                    {
                        temp_copia[0] = 3;
                        temp_copia[1] = 0;
                    }
                    
                    if(temp_copia[0] < 1 && temp_copia[1] >= 0)
                    {
                        temp_copia[0] = 1;
                        temp_copia[1] = 0;
                    }
                } 
        }
        estado_anterior = estado_actual;
}

void envio_data()
{
    static char ident = 0;
    char vector_datos[15] = {'D'};
    char contador = 0;
    temp_enviar[0] = 'M';
    
    for(int l = 0; l < max_disp; l++)
    {
        for(int i = 0; i < tipos_disp; i++)
        {
            if(nombre_disp[l][i][0] != '\0')
            {
                if(est_disp_enviar[i][l] != est_disp[i][l])
                {
                    ident = 1;
                    est_disp_enviar[i][l] = est_disp[i][l];
                }
            }
        }
    }
    
    for(int i = 0; i < 2; i++)
    {
        if(temp_enviar[i+1] != temp_deseada[i]+48)
        {
            temp_enviar[i+1] = temp_deseada[i]+48;
            ident = 2;
        }
    }
    
    switch(ident)
    {
        case 0:
        break;
        
        case 1:
            if(modificador != 1)
            {
                for(int l = 0; l < max_disp; l++)
                {
                    for(int i = 0; i < tipos_disp; i++)
                    {
                        if(nombre_disp[l][i][0] != '\0')
                        {
                            contador++;
                            vector_datos[contador] = est_disp[i][l]+48;
                        }
                    }
                }
                vector_datos[contador] = '\0';
                envio_trama(vector_datos);
                ident = 0;
            }
        break;
        
        case 2:
            envio_trama(temp_enviar);
            ident = 0;
        break;        
    }
}