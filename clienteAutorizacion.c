/*******************************************************************************/
/* SERVIDOR ClienteAutorizador:                                                */
/* Servicios relacionados con la validación de la clave Internet               */
/*                                                                             */
/*******************************************************************************/
/* Omega Ingenieria de Software Ltda                                           */
/* Fecha de Desarrollo  : 19 de Abril del 2002                                 */
/* Equipo de Desarrollo : Danilo A. Leiva Espinoza, Jan Riega  Z.,             */ 
/*			  Boris Contreras Mac-lean                             */
/* Ultima Modificacion  :                                                      */
/*******************************************************************************/
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>

#include <atmi.h>           /* TUXEDO Header File API's */
#include <userlog.h>        /* TUXEDO Header File */
#include <fml32.h>            /* TUXEDO FML Support */

#include <hermesFML.h>
#include <clienteFML.h>

#include <olist.h>
#include <GestionDeErrores.h>
#include <servidor.h>
#include <buffer.h>
#include <omegafec2.h>
#include <omegafin2.h>
#include <atalla_f2.h>

#include <cliente.h>
#include <clienteAutorizacion_fun.h>
#include <clienteAutorizacion_sql.h>

/* Variables globales de parametros de proceso */
/* Se accesan al levantar el servidor        */
char gFechaProceso[DIM_FECHA];
char gFechaCalendario[DIM_FECHA];
short gCodigoMonedaPais;

tpsvrinit(int argc, char *argv[])
{
   long sts;
   int  tpbeginSts;

   if (tpopen() == -1)
   {
      userlog("El servidor fallo al conectarse con el Administrador de Base de Datos");
      exit(1);
   }

   tpbeginSts = tpbegin(10L,0L);
   if (tpbeginSts == -1) {
       userlog("tpsvrinit: Falla tpbegin");
       exit(1);
   }

   sts = RecuperarParametrosProceso(gFechaProceso,
                                    gFechaCalendario,
                                    &gCodigoMonedaPais);
   if (sts != SRV_SUCCESS)
   {
      userlog("tpsvrinit: Falla recuperacion de parametros de proceso (SGT)");
      tpabort(0L);
      exit(1);
   }

   tpcommit(0L);

   userlog("El servidor ha iniciado su ejecucion satisfactoriamente");
   userlog("Fecha de proceso:%s Fecha Calendario:%s\n", gFechaProceso, gFechaCalendario);
   return 0;
}

void tpsvrdone()
{
   if (tpclose() == -1)
   {
      switch(tperrno)
      {
         case TPERMERR:
            userlog("El servidor fallo al desconectarse del Administrador de Base de Datos para mas informacion consultar al manejador especifico");
            break;

         case TPEPROTO:
            userlog("El servidor fallo al desconectarse del Administrador de Base de Datos debido a un problema de contexto del close()");
            break;

         case TPESYSTEM:
            userlog("El servidor fallo al desconectarse del Administrador de Base de Datos debido a un problema con Tuxedo-System/T");
            break;

         case TPEOS:
            userlog("El servidor fallo al desconectarse del Administrador de Base de Datos debido a un error del Sistema Operativo");
            break;

         default:
            userlog("El servidor fallo al desconectarse del Administrador de Base de Datos debido a un error de excepcion en el sistema");
            break;
      }
          return;
   }
   userlog("El servidor ha finalizado su ejecucion...");
   return;
}

/************************************************************************/
/* Objetivo: Valida la clave Internet ingresada                         */
/* Autor   : Boris Contreras Mac-lean                 Fecha: 06-05-2002 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliAutValPinInt(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliCLAVES claves;
   tCliCLAVEDEACCESOINTERNET claveAcceso;
   long rutIngresado;
   char pinIngresado[CLI_LARGO_PIN_INTERNET + 1];
   long sts;
   long retorno;
   short sucursal=1;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts =  Fget32(fml, CLI_RUT ,          	0, (char *)&rutIngresado, 0);  
   sts |= Fget32(fml, CLI_PIN_INTERNET, 	0, pinIngresado, 0);    

   if (Foccur32(fml, CLI_SUCURSAL) > 0)
      sts |= Fget32(fml, CLI_SUCURSAL,               0, (char *)&sucursal, 0);

   RegistrarLog(fml, rqst->name, rutIngresado, sucursal);

   if (sts != 1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en la obtención de datos de entrada.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CRITICAL_ERROR ,(char *)fml, 0L, 0L);
   }

   /*-- Se recuperan las llaves de encriptación --*/ 

   sts = RecuperarLlavesEncriptacion(&claves);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar las llaves de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = DesencriptarLlaves(&claves);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener las llaves de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   retorno = SRV_SUCCESS;

   sts = ValidarClientePinInternet(claves, rutIngresado, pinIngresado);
   if (sts != SRV_SUCCESS && sts != SRV_CLI_MODIFICAR_PIN)
   {
      switch(sts)
      {
	    case SRV_NOT_FOUND:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "No posee clave de acceso.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_NOT_FOUND, (char *)fml, 0L, 0L);
               break;

	    case SRV_CLI_PIN_BLOQUEADO:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Su clave ha sido bloqueada, contáctese con su ejecutivo de cuenta.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_CLI_PIN_BLOQUEADO, (char *)fml, 0L, 0L);
               break;

            case SRV_CLI_PIN_NO_VALIDO:

	       claveAcceso.rutCliente = rutIngresado;

               sts = SqlCliIncrementarIntentosFallidosClaveInternet(&claveAcceso);
               if (sts != SQL_SUCCESS)
               {
                  Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al incrementar los accesos fallidos", 0);
                  TRX_ABORT(transaccionGlobal);
                  tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
               }

               if (claveAcceso.intentosFallidos >= CLI_CANTIDAD_MAXIMA_ACCESO_FALLIDOS)
               {
                   sts = SqlCliBloquearClaveDeAccesoInternet(&claveAcceso);
                   if (sts != SQL_SUCCESS)
                   {
                      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al bloquear clave de acceso", 0);
                      TRX_ABORT(transaccionGlobal);
                      tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
                   }
               }

	       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clave de acceso inválida.", 0);
               TRX_COMMIT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
               break;

	    default:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar clave de acceso.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

   claveAcceso.rutCliente = rutIngresado;

   sts = SqlCliReiniciarIntentosFallidosClaveInternet(&claveAcceso);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar accesos fallidos", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   retorno = (sts == SRV_CLI_MODIFICAR_PIN)? SRV_CLI_MODIFICAR_PIN:SRV_SUCCESS;


   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, retorno, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Valida la clave IVR ingresada                              */
/* Autor   : Boris Contreras Mac-lean                 Fecha: 09-05-2002 */
/* Modifica: Consuelo Montenegro Arellano             Fecha: 17-07-2002 */
/*           Incorporacion de incrementar intentos fallidos ivr         */
/************************************************************************/
void CliAutValPinIvr(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliCLAVES claves;
   tCliCLAVEDEACCESOIVR claveAcceso;
   long rutIngresado;
   char pinIngresado[CLI_LARGO_PIN_IVR + 1];
   long sts;
   long retorno;
   short sucursal=1;

   /* TVT. 13-10-2009. Validacion Pin en Claro. In. */
   char Respuesta[LARGO_RESPUESTA];
   int ret;
   int retSW;
   int retHW;
   int ResultadoValidacion=0;
   char campoSw[2+1];
   char campoHw[2+1];
   strcpy(campoSw,"SW");
   strcpy(campoHw,"HW");
   /* TVT. 13-10-2009. Validacion Pin en Claro. Fn. */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts =  Fget32(fml, CLI_RUT ,      0, (char *)&rutIngresado, 0);
   sts |= Fget32(fml, CLI_PIN_IVR,   0, pinIngresado, 0);

   if (Foccur32(fml, CLI_SUCURSAL) > 0)
      sts |= Fget32(fml, CLI_SUCURSAL, 0, (char *)&sucursal, 0);

   RegistrarLog(fml, rqst->name, rutIngresado, sucursal);

   if (sts != 1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en la obtención de datos de entrada.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CRITICAL_ERROR ,(char *)fml, 0L, 0L);
   }

   /*-- Se recuperan las llaves de encriptación --*/
   /* TVT. 13-10-2009. Validacion Pin en Claro. */
   /* Si Validacion Software = 1 */
   if (retornoTipoValidar(campoSw))
   {
	sts = RecuperarLlavesEncriptacion(&claves);
	if (sts != SRV_SUCCESS)
	{
	      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar las llaves de encriptación.", 0);
	      TRX_ABORT(transaccionGlobal);
	      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	}

	sts = DesencriptarLlaves(&claves);
	if (sts != SRV_SUCCESS)
	{
	      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener las llaves de encriptación.", 0);
	      TRX_ABORT(transaccionGlobal);
	      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	}
   }

   /* TVT. 13-10-2009. Validacion Pin en Claro. */
   /* Si Validacion Software = 1 */
   if (retornoTipoValidar(campoSw))
   {
	sts = ValidarClientePinIvr(claves, rutIngresado, pinIngresado);
   } 
   else	/* Si Validacion Hardware = 1 */
   {
   	sts = ValidarClientePinF2Ivr(rutIngresado, pinIngresado);
   }
   
   if (sts != SRV_SUCCESS && sts != SRV_CLI_MODIFICAR_PIN)
   {
         switch(sts)
         {
            case SRV_NOT_FOUND:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "No posee clave de acceso.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_NOT_FOUND, (char *)fml, 0L, 0L);
               break;

            case SRV_CLI_PIN_BLOQUEADO:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Su clave ha sido bloqueada, contáctese con su ejecutivo de cuenta.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_CLI_PIN_BLOQUEADO, (char *)fml, 0L, 0L);
               break;

            case SRV_CLI_PIN_NO_VALIDO:

               claveAcceso.rutCliente = rutIngresado;

               sts = SqlCliIncrementarIntentosFallidosClaveIvr(&claveAcceso);
               if (sts != SQL_SUCCESS)
               {
                  Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al incrementar los accesos fallidos", 0);
                  TRX_ABORT(transaccionGlobal);
                  tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
               }

               if (claveAcceso.intentosFallidos >= CLI_CANTIDAD_MAXIMA_ACCESO_FALLIDOS)
               {
                   sts = SqlCliBloquearClaveDeAccesoIvr(&claveAcceso);
                   if (sts != SQL_SUCCESS)
                   {
                      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al bloquear clave de acceso", 0);
                      TRX_ABORT(transaccionGlobal);
                      tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
                   }
               }

               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clave de acceso inválida.", 0);
               TRX_COMMIT(transaccionGlobal);
               tpreturn(TPFAIL, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
               break;

            default:
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar clave de acceso.", 0);
               TRX_ABORT(transaccionGlobal);
               tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
   }

   claveAcceso.rutCliente = rutIngresado;

   sts = SqlCliReiniciarIntentosFallidosClaveIvr(&claveAcceso);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar accesos fallidos", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/***********************************************************************************/
/* Objetivo: Calcula el Offset internet dado el rut del usuario y el Pin Internet. */
/* Autor   : Danilo A. Leiva Espinoza                           Fecha: 02-05-2002  */
/* Modifica: Boris Contreras Mac-lean				Fecha: 10-05-2002  */
/***********************************************************************************/
void CliCalPinOffInt(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int  transaccionGlobal;
   tCliCRIPTOGRAMA criptograma;
   char  tablaDecimalizacion[CLI_LARGO_TABLA_DECIMAL + 1];
   char  pinIngresado[CLI_LARGO_PIN_INTERNET + 1];
   char  pinOffset[CLI_LARGO_BLOCK_INTERNET + 1];
   long  pinNatural;
   long  rutIngresado;
   long  sts;

       CLI_OPCION_INDICA_INTERNET;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts =  Fget32(fml, CLI_RUT,             0, (char *)&rutIngresado, 0);
   sts |= Fget32(fml, CLI_PIN_INTERNET,    0, pinIngresado, 0);

   if (sts != 1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en la obtención de datos de entrada.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, TPFAIL ,(char *)fml, 0L, 0L);
   }

   if( strlen(pinIngresado) != CLI_LARGO_PIN_INTERNET)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el formato exigido de su clave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CLI_LARGO_PIN_NO_VALIDO,(char *)fml, 0L, 0L);  
   }

   criptograma.identificador = CLI_CRIPTOGRAMA_PIN_INTERNET;

   sts = RecuperarCriptograma(&criptograma);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = DesencriptarCriptograma(&criptograma);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }


   sts = RecuperarTablaDecimalizacion(tablaDecimalizacion);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar tabla de decimalización.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = DesencriptarLlave(tablaDecimalizacion);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = ObtenerPinNatural(rutIngresado, criptograma.criptograma, tablaDecimalizacion, CLI_OPCION_INDICA_INTERNET, &pinNatural);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener pin natural", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   CalcularPinOffsetInternet(pinNatural, pinIngresado, pinOffset);

   Fadd32(fml, CLI_PIN_OFFSET, pinOffset, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/***********************************************************************************/
/* Objetivo: Calcula el Ofseet ivr dado el rut del usuario y el Pin Ivr.           */
/* Autor   : Danilo A. Leiva Espinoza                           Fecha: 02-05-2002  */
/* Modifica: Boris Contreras Mac-lean                           Fecha: 10-05-2002  */
/***********************************************************************************/
void CliCalPinOffIvr(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int  transaccionGlobal;
   tCliCRIPTOGRAMA criptograma;
   char  tablaDecimalizacion[CLI_LARGO_TABLA_DECIMAL + 1];
   char  pinIngresado[CLI_LARGO_PIN_IVR + 1];
   char  pinOffset[CLI_LARGO_PIN_IVR + 1];
   long  pinNatural;
   long  pinUsuario;
   long  rutIngresado;
   long  sts;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts =  Fget32(fml, CLI_RUT,        0, (char *)&rutIngresado, 0);
   sts |= Fget32(fml, CLI_PIN_IVR,    0, pinIngresado, 0);
 
   if (sts != 1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en la obtención de datos de entrada.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, TPFAIL ,(char *)fml, 0L, 0L);
   }

   if( strlen(pinIngresado) != CLI_LARGO_PIN_IVR)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el formato exigido de su clave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CLI_LARGO_PIN_NO_VALIDO,(char *)fml, 0L, 0L);  
   }

   criptograma.identificador = CLI_CRIPTOGRAMA_PIN_IVR;

   sts = RecuperarCriptograma(&criptograma);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = DesencriptarCriptograma(&criptograma);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = RecuperarTablaDecimalizacion(tablaDecimalizacion);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar tabla de decimalización.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = DesencriptarLlave(tablaDecimalizacion);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener la llave de encriptación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = ObtenerPinNatural(rutIngresado, criptograma.criptograma, tablaDecimalizacion, CLI_OPCION_INDICA_IVR, &pinNatural);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al obtener pin natural", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   pinUsuario = atol(pinIngresado);

   CalcularPinOffsetIvr(pinNatural, pinUsuario, pinOffset);

   Fadd32(fml, CLI_PIN_OFFSET, pinOffset, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
