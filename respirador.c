/*
 CONTROLADOR PARA RESPIRADOR modelo ARSE v1.4

 Copyright 2020 Enrique Alapont Puchalt
 Por la presente se concede permiso, libre de cargos, a cualquier persona que obtenga una copia de
 este software y de los archivos de documentación asociados (el "Software"), a utilizar el Software
 sin restricción, incluyendo sin limitación los derechos a usar, copiar, modificar, fusionar, publicar,
 distribuir, sublicenciar, y/o vender copias del Software, y a permitir a las personas a las que se les
 proporcione el Software a hacer lo mismo, sujeto a las siguientes condiciones:
 El aviso de copyright anterior y este aviso de permiso se incluirán en todas las copias o partes
 sustanciales del Software.

 EL SOFTWARE SE PROPORCIONA "COMO ESTÁ", SIN GARANTÍA DE NINGÚN TIPO, EXPRESA O IMPLÍCITA, INCLUYENDO
 PERO NO LIMITADO A GARANTÍAS DE COMERCIALIZACIÓN, IDONEIDAD PARA UN PROPÓSITO PARTICULAR E INCUMPLIMIENTO.
 EN NINGÚN CASO LOS AUTORES O PROPIETARIOS DE LOS DERECHOS DE AUTOR SERÁN RESPONSABLES DE NINGUNA
 RECLAMACIÓN, DAÑOS U OTRAS RESPONSABILIDADES, YA SEA EN UNA ACCIÓN DE CONTRATO, AGRAVIO O CUALQUIER OTRO
 MOTIVO, DERIVADAS DE, FUERA DE O EN CONEXIÓN CON EL SOFTWARE O SU USO U OTRO TIPO DE ACCIONES EN EL SOFTWARE.
 (licencia MIT)

 ··········································································
 : : .
 : +----------------+ +------------+ :
 : | A4|-------------|SDA LCD | :
 : | A5|-------------|SCL 2x16 | :
 : | | | | :
 : | | +---|GND VCC|----> +5 +5 :
 : | | | +------------+ ^ : ····················································
 : | | === | : : :
 : | | +---O--------------O---------+----------+----------+ :
 : | ARDUINO | : : | | | :.
 : | NANO | : : +++ +++ +++ :
 : | | : : P1 | | P2 | | P3 | | :
 : | A0|----------------------------------------------------O--------------O------->| | +---->| | +---->| | 3 x 47K LINEAL :
 : | | +---O--------------O----+ | | | | | | | | (4K7~500K) :
 : | | | : : | +++ | +++ | +++ :
 : | | === : : | | | | | | :
 : | | : : +----+---|------+---|------+ :
 : | A1|----------------------------------------------------O--------------0-------------+ | :
 : | A2|----------------------------------------------------O--------------0------------------------+ :
 : | | : : :
 : | D7|-- salidaINSP +12 : ····················································
 : | D8|-- salidaESP ^ : tarjeta de potenciómetros
 : | | | : rojo +------------+
 : | | +--------+ B +--------+ C +-------------O-------------------|POS |
 : | D3|--+ 1K +------| TIP120 |------------------------O-------------------|NEG |
 : | GND | +--------+ +----+---+ : negro +------------+
 : +-------+--------+ | E : soplador
 : | === :
 : === :
 : :
 : +12 +5 :
 : ^ ^ :
 : | | :
 +-------+ +12V : | 1 +------+ 3 | :
 230 -----| ~/= |-------D---------------------------+------+----+ 7805 +----+ :
 Vca -----| |-------D----+ | +--+---+ | :
 +-------+ : | ___ |2 --- :
- fuente de : === 33uF/50 V --- === --- 100 nF/15 V :
 alimentación : | | :
 12V/5A : === === :
 ··········································································
 tarjeta electrónica
 **********************************************************************************************************************
 * MEMORIA DE CAMBIOS *
 * *
 * FECHA VERSIÓN NATURALEZA DEL CAMBIO *
 *========== ======= =================================================================================================*
 *2020-03-21 1.0 diseño original *
 *2020-03-23 1.1 se le incorpora PEEP *
 * se añade estabilizador 7805 *
 *2020-03-26 1.2 intercambio funciones de las rutinas bucle y RSI *
 *2020-03-28 1.3 añado nuevos estados *
 *2020-04-01 1.4 adopto el nombre íbero de Sagunto como nombre del proyecto *
 * visualiza tiempos en segundos, no en milisegundos *
 **********************************************************************************************************************
 ¡TODOS LOS TIEMPOS ESTÁN EN MILISEGUNDOS!
*/
//******************************************* LLAMA A BIBLIOTECAS E INICIALIZA LCD
 #include <TimerOne.h>
 #include <Wire.h>
 #include <LiquidCrystal_I2C.h>
 LiquidCrystal_I2C lcd(0x27, 16, 2);
//******************************************* VARIABLES, PARÁMETROS Y CONSTANTES.
//------------------------------------------- POTENCIAS
long Potencia = 0 ; //-------------------- 0~255 para sacar por puerto PWM
long PotenciaI0 = 255; //------------------- 0~255 para mantener presión positiva en la espiración
long PotenciaI1; //------------------- 0~255
long PotenciaE0 = 63; //------------------ 0~255
long PotenciaE1 = 63; //------------------ 0~255
long PotenciaPorCien; //-------------- 0~99 para sacar por el LCD
long maxPotencia = 255; //-------------- 0~255 máxima potencia (P1 a la derecha). Cambiar solo si montamos una turbina sobredimensionada. SOLO PARA EL INTERVALO I1
long minPotencia = 0; //-------------- 0~255 mínima potencia (P1 a la izquierda). Cambiar solo si lo dicen los médicos. SOLO PARA EL INTERVALO I1
//------------------------------------------- TIEMPOS
long leoPotencia = analogRead(A0); //------ 0~1023 para leer potenciómetro de potencia
long leoTiempoInsp = analogRead(A1); //------ 0~1023 para leer potenciómetro de tiempo total de inspiración
long leoTiempoEsp = analogRead(A2); //------ 0~1023 para leer potenciómetro de tiempo total de espiración
long TiempoI1 = 200; //-------------- Definición del tramo I0 (ms, %) hasta Tiempo Inspiración
long TiempoInsp; //------------------------- tiempo total de inspiración total, ms
float floTiempoInsp; //-------------- flotante del anterior para obtener decimales
long MaxTiempoInsp = 4000; //-------------- máximo tiempo de inspiración (P2 a la derecha)
long MinTiempoInsp = 1000; //-------------- mínimo tiempo de inspiración (P2 a la izquierda)
long TiempoEsp; //------------------------- tiempo total de espiración
float floTiempoEsp; //--------------- flotante del anterior para obtener dwecimales
long MaxTiempoEsp = 6000; //--------------- máximo tiempo de inspiración (P3 a la derecha)
long MinTiempoEsp = 1000; //--------------- mínimo tiempo de inspiración (P3 a la izquierda)
long TiempoSumaE1; // -------------- tiempo absoluto de E1
long TiempoE0 = 1000; //--------------- intervalo E0 hasta E1
long RPM; //------------------------- respiraciones por minuto
long MaxRPM = 30; //------------------------- máximas RPM (P3 a la derecha)
long MinRPM = 10; //------------------------- mínimas RPM (P3 a la izquierda)
long Periodo; //------------------------- periodo de respiración, ms. Calculado
float RIE; //------------------------- relación I:E calculada para pasar al display
long TicTac = 100; //---------- variable cronométrica que recorre el tiempo de respiración
//------------------------------------------- PINES DE SALIDA
byte salidaPWM = 3; //---------- pin de salida hacia el driver del motor
byte salidaINS = 7; //---------- indicador de inspiración. NO SE USA de momento
byte salidaESP = 8; //---------- indicador de espiración. NO SE USA de momento
String Estado = "--"; //---------- indicador de estado temporal
void setup() //****************************** SETUP
{
 //--------------------- define pines de entrada y salida
 pinMode(A0, INPUT); //--------------------- potenciómetro P1 POTencia
 pinMode(A1, INPUT); //--------------------- potenciómetro P2 tiempo de INSpiración
 pinMode(A2, INPUT); //--------------------- potenciómetro P3 TIEMPO DE ESpiración

 pinMode(salidaINS, OUTPUT); //------------- salida señal inspiración
 pinMode(salidaESP, OUTPUT); //------------- salida señal espiración
 pinMode(salidaPWM, OUTPUT); //------------- salida PWM hacia el driver de la turbina
 //----------------------------------------- inicializa el LCD
 lcd.init();
 lcd.backlight();
 //----------------------------------------- inicializa interrupciones
 noInterrupts();//-------------------------- paramos todas las interrupciones antes de configurar un timer
 Timer1.initialize(100000); //---------- <--= el único que va en microsegundos
 Timer1.attachInterrupt(RSI);
 interrupts();
lcd.clear(); //------------------------------------------------------------------- borra LCD y pinta
 lcd.setCursor(0, 0); lcd.print("Respirador ARSE"); //----------------------------- prepara primera línea y escribe cosas fijas
 lcd.setCursor(0, 1); lcd.print("2020-04-01 V1.4"); //----------------------------- prepara segunda línea y escribe cosas fijas
 delay(2000);

} //**************************************************************************************** fin SETUP
void loop() {/****************************************************************************** BUCLE PRINCIPAL
 ******************************************************************************* lee potenciómetros y visualiza en el LCD */
 leoPotencia = analogRead(A0); //---------------------------------------------------- lee P1
 leoTiempoInsp = analogRead(A1); //---------------------------------------------------- lee P2
 leoTiempoEsp = analogRead(A2); //---------------------------------------------------- lee P3

 PotenciaI1 = minPotencia + (maxPotencia - minPotencia) * leoPotencia / 1024 ; //-- 0~256 pasa rango de 1024 a 256 para el PWM (0~255)
 PotenciaPorCien = PotenciaI1 * 100 / 256; //-- pasa rango de 256 a 100 para el LCD (en %)
 TiempoInsp = MinTiempoInsp + (MaxTiempoInsp - MinTiempoInsp) * leoTiempoInsp / 1024;
 TiempoEsp = MinTiempoEsp + (MaxTiempoEsp - MinTiempoEsp) * leoTiempoEsp / 1024;
 TiempoSumaE1 = TiempoInsp + TiempoE0;
 Periodo = TiempoInsp + TiempoEsp;
 RPM = 60000 / Periodo; //--------------------------------------------------------- pasa de ms a respiraciones por minuto
 floTiempoInsp = TiempoInsp; floTiempoEsp = TiempoEsp; //-------------------------- pasa a flotante para poder visualizar decimales
 floTiempoInsp = floTiempoInsp/1000; floTiempoEsp = floTiempoEsp/1000; //---------- pasa a de milisegundos a segundos
 RIE = floTiempoEsp / floTiempoInsp ; //-------------------------------------------- relación I:E
 lcd.clear(); //------------------------------------------------------------ borra LCD y pinta
 lcd.setCursor(0, 0); lcd.print("P R i "); //------------------------------ prepara primera línea y escribe cosas fijas
 lcd.setCursor(0, 1); lcd.print("I:E 1: e "); //------------------------------ prepara segunda línea y escribe cosas fijas
 //-------------------------------------------- escribe cosas variables en el display
 lcd.setCursor(8, 0); lcd.print(Estado);
 lcd.setCursor(12, 0); lcd.print(floTiempoInsp);
 lcd.setCursor(12, 1); lcd.print(floTiempoEsp);
 lcd.setCursor(1, 0); lcd.print(PotenciaPorCien);
 lcd.setCursor(6, 1); lcd.print(RIE);
 lcd.setCursor(5, 0); lcd.print(RPM);

 delay (100);
} //************************************************************************************ fin BUCLE PRINCIPAL
void RSI() { /* *********************************************************************************** RSI, rutina de servicio a la interrupción

 |<--TiempoI1-->| |<-TiempoE0-->|
 |<--------TiempoInsp-------->|<--------TiempoEsp-------->|
 |<--------------------TiempoSumaE1-------->|
 |<---------------------------------Periodo-------------->|
 ESTADO | I0 | I1 | E0 | E1 |
 POTENCIA | PotenciaI0 | PotenciaI1 | PotenciaE0 | Potencia E1 | */
 TicTac = TicTac + 100; //-------------------------------------------------------------- variable cronométrica usada para recorrer el periodo de respiración

 if (TicTac < TiempoI1) //------------------------------------------------------------------ comienza estado E0
 {
 Estado= "I0"; analogWrite(salidaPWM, PotenciaI0); //------------------------------------ saca potencia programada por la salida PWM
 digitalWrite(0, HIGH); digitalWrite(LED_BUILTIN, HIGH); digitalWrite(1, LOW); //-------- escribe en las salidas auxiliares (no se usan, de momento) y en el LED
 return;
 }

 if (TicTac > TiempoI1 && TicTac < TiempoInsp) //--------------------------------- termina E0, comienza E1
 {
 Estado= "I1"; analogWrite(salidaPWM, PotenciaI1);
 digitalWrite(0, HIGH); digitalWrite(LED_BUILTIN, HIGH); digitalWrite(1, LOW);
 return;
 }

 if (TicTac > TiempoInsp && TicTac < TiempoSumaE1) //---------------------------------- termina E1 y la inspiración, comienza E0
 {
 Estado= "E0"; analogWrite(salidaPWM, PotenciaE0); //---------------------------------- PotenciaE0 es la primera fase del PEEP
 digitalWrite(0, LOW); digitalWrite(LED_BUILTIN, LOW); digitalWrite(1, HIGH);
 return;
 }
 if (TicTac > TiempoSumaE1 && TicTac < Periodo) //------------------------------------- Tiempo E1 empieza estado E1
 {
 Estado= "E1"; analogWrite(salidaPWM, PotenciaE1);
 digitalWrite(0, LOW); digitalWrite(LED_BUILTIN, LOW); digitalWrite(1, HIGH); //---------- PotenciaE1 es la segunda fase del PEEP
 return;
 }
 Periodo = TiempoInsp + TiempoEsp;
 if (TicTac > Periodo) { TicTac = 0; return; } //----------------------------------- Periodo de respiración terminado. FIN

}// *************************************************************************************** TERMINA RSI
// *************************************************************************************** ¡Y ESTO ES TÓ, ESTO ES TÓ, ESTO ES TODO, AMIGOS