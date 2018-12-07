/************************************************************************/
/*                             CLIENTE                                  */
/*                      SERVIDOR:cliEnrolamientoTrx                     */
/*                                                                      */
/************************************************************************/
/* Omega Ingenieria de Software Ltda                                    */
/* Fecha de Desarrollo  : Octubre de 2004                               */
/************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <oci.h>
#include <malloc.h>
#include <atmi.h>           /* TUXEDO Header File API's */
#include <userlog.h>        /* TUXEDO Header File */
#include <fml32.h>          /* TUXEDO FML Support */


#include <servidor.h>
#include <GestionDeErrores.h>

#include <clienteFML.h>
#include <custodiaFML.h>
#include <hermesFML.h>
#include <sgtFML.h>

#define  ESTADO_CUSTODIA_EN_EJECUTIVO    10

#define  COD_DOC_FICHA_ENR    1
#define  COD_DOC_FICHA_DOC    2
#define  COD_DOC_FICHA_CUS   11

#define  COD_DOC_MANDATO_ENR  2
#define  COD_DOC_MANDATO_DOC  1
#define  COD_DOC_MANDATO_CUS  8

#define  COD_DOC_REGFIR_ENR   3
#define  COD_DOC_REGFIR_DOC   4
#define  COD_DOC_REGFIR_CUS   9

#define  COD_DOC_CONDSEG_ENR  4
#define  COD_DOC_CONDSEG_DOC  3
#define  COD_DOC_CONDSEG_CUS  3

tpsvrinit(int argc, char *argv[])
{
   if (tpopen() == -1)
   {
      userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al conectarse con el Administrador de Base de Datos");
      exit(1);
   }

   userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", ha iniciado su ejecucion satisfactoriamente...");
   srandom( (unsigned)time( NULL ) );
   return 0;
}


void tpsvrdone()
{
   if (tpclose() == -1)
   {
      switch(tperrno)
      {
         case TPERMERR:
            userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al desconectarse del Administrador de Base de Datos para mas informacion consultar al manejador especifico");
            break;

         case TPEPROTO:
            userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al desconectarse del Administrador de Base de Datos debido a un problema de contexto del close()");
            break;

         case TPESYSTEM:
            userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al desconectarse del Administrador de Base de Datos debido a un problema con Tuxedo-System/T");
            break;

         case TPEOS:
            userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al desconectarse del Administrador de Base de Datos debido a un error del Sistema Operativo");
            break;

         default:
            userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", fallo al desconectarse del Administrador de Base de Datos debido a un error de excepcion en el sistema");
            break;
      }
	  return ;
   }
   userlog("El Servidor de Cliente: \"cliEnrolamientoTrx\", ha finalizado su ejecucion...");
   return ;
}

/************************************************************************/
/* Objetivo: Registrar documentos en DocumentacionFirmada y Custodia.   */
/* Autor   : Trebol-it                                Fecha: 14-10-2004 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRegDocFirCus(TPSVCINFO *rqst)
{
   FBFR32 *fml, *fmlAux;
   int transaccionGlobal;
   long sts=0;

   long   rutCliente;
   char   dv[1 + 1];
   short  sucursal;
   long   usuario;
   char   fecha[8 + 1];

   short  tipoDocumento, tipoDocumentoAux;
   char   codigoDeBarra[50+1];
   short  nroDeRegistros;
   short  i;
   long   largo;
   short  estadoCustodia = ESTADO_CUSTODIA_EN_EJECUTIVO;
   long   consignatarioCustodia = 0L;
   char   fechaCalendario[14 + 1];
/* Control existencia Contrato Unico [INI] */
   short existeCU;
/* Control existencia Contrato Unico [FIN] */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT,            0, (char *)&rutCliente, 0);
   Fget32(fml, CLI_DIGVER,         0,          dv, 0);
   Fget32(fml, CLI_SUCURSAL,       0, (char *)&sucursal, 0);
   Fget32(fml, CLI_USUARIO,        0, (char *)&usuario, 0);
   if (Foccur32(fml, CLI_FECHA) > 0)
      Fget32(fml, CLI_FECHA,          0,          fecha, 0);
   else
   { 
      fmlAux = (FBFR32 *)tpalloc("FML32",NULL, 1024);
      if (tpcall("SgtRecParametro", (char *)fmlAux, 0L, (char **)&fmlAux, &largo, TPNOTRAN) == -1)
      {
         tpfree((char *)fmlAux);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar fecha calendario.", 0);
         tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
      Fget32(fmlAux, SGT_FECHA, 0, fechaCalendario, 0);
      tpfree((char *)fmlAux);
      strncpy(fecha, fechaCalendario, 8); fecha[8] = '\0';
   }

/* Control existencia Contrato Unico [INI] */
   fmlAux = (FBFR32 *)tpalloc("FML32",NULL, 1024);
   Fadd32(fmlAux, CUS_RUTCLIENTE, (char *)&rutCliente, 0);
   if (tpcall("CusRecFolioXRut", (char *)fmlAux, 0L, (char **)&fmlAux, &largo, TPNOTRAN) == -1)
   {
      if (tpurcode == SRV_NOT_FOUND)
      {
         tpfree((char *)fmlAux);
         existeCU = 0;
      }
      else
      {
         tpfree((char *)fmlAux);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al verificar existencia de Contrato Unico", 0);
         tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   }
   else
   {
      tpfree((char *)fmlAux);
      existeCU = 1;
   }
/* Control existencia Contrato Unico [FIN] */

   nroDeRegistros = Foccur32(fml, CLI_TIPO_DOCUMENTO);


   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   for (i=0; i < nroDeRegistros; i++)
   {
      Fget32(fml, CLI_TIPO_DOCUMENTO, i, (char *)&tipoDocumento, 0);

      /* Registro en DocumentacionFirmada */
      switch(tipoDocumento)
      {
       case COD_DOC_FICHA_ENR:
          tipoDocumentoAux = COD_DOC_FICHA_DOC;
          break;

       case COD_DOC_MANDATO_ENR:
          tipoDocumentoAux = COD_DOC_MANDATO_DOC;
          break;

       case COD_DOC_REGFIR_ENR:
          tipoDocumentoAux = COD_DOC_REGFIR_DOC;
          break;

       case COD_DOC_CONDSEG_ENR:
          tipoDocumentoAux = COD_DOC_CONDSEG_DOC;
          break;

       default:
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Tipo de documento invalido.", 0);
          TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
          break;
      }

      fmlAux = (FBFR32 *)tpalloc("FML32",NULL, 1024);
      Fadd32(fmlAux, CLI_RUT, (char *)&rutCliente, 0);
      Fadd32(fmlAux, CLI_TIPO_DOCUMENTO, (char *)&tipoDocumentoAux, 0);
      if (tpcall("CliInsDocFirInt", (char *)fmlAux, 0L, (char **)&fmlAux, &largo, 0L) == -1)
      {
         tpfree((char *)fmlAux);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al registrar en documentacion firmada.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
      tpfree((char *)fmlAux);

      /* Registro en Custodia */
      switch(tipoDocumento)
      {
       case COD_DOC_FICHA_ENR:
          tipoDocumentoAux = COD_DOC_FICHA_CUS;
          sprintf(codigoDeBarra,"%02d%09li%s%s", tipoDocumentoAux, rutCliente, dv, fecha);
          break;

       case COD_DOC_MANDATO_ENR:
          tipoDocumentoAux = COD_DOC_MANDATO_CUS;
          sprintf(codigoDeBarra,"%02d%09li%s%s", tipoDocumentoAux, rutCliente, dv, fecha);
          break;

       case COD_DOC_REGFIR_ENR:
          tipoDocumentoAux = COD_DOC_REGFIR_CUS;
          sprintf(codigoDeBarra,"%02d%09li%s%s", tipoDocumentoAux, rutCliente, dv, fecha);
          break;

       case COD_DOC_CONDSEG_ENR:
          tipoDocumentoAux = COD_DOC_CONDSEG_CUS;
          sprintf(codigoDeBarra,"%02d%09li%s%s", tipoDocumentoAux, rutCliente, dv, fecha);
          break;

       default:
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Tipo de documento invalido.", 0);
          userlog("%s. Tipo de documento invalido (documento %d)", rqst->name, tipoDocumento);
          TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
          break;
      }

/* Control existencia Contrato Unico [FIN] */
      if (!existeCU || tipoDocumento == COD_DOC_REGFIR_ENR)
      {
         fmlAux = (FBFR32 *)tpalloc("FML32",NULL, 1024);
         Fadd32(fmlAux, CUS_CODIGODEBARRA, codigoDeBarra, 0);
         Fadd32(fmlAux, CUS_ESTADOCUSTODIA, (char *)&estadoCustodia, 0);
         Fadd32(fmlAux, CUS_SUCURSALINGRESO, (char *)&sucursal, 0);
         Fadd32(fmlAux, CUS_EJECUTIVOINGRESO, (char *)&usuario, 0);
         Fadd32(fmlAux, CUS_CONSIGNATARIOINGRESO, (char *)&consignatarioCustodia, 0);
         if (tpcall("CusInsEstCus", (char *)fmlAux, 0L, (char **)&fmlAux, &largo, 0L) == -1)
         {
            tpfree((char *)fmlAux);
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al registrar en Custodia.", 0);
            userlog("%s. Error al registrar en Custodia (documento %d)", rqst->name, tipoDocumento);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
         }
         tpfree((char *)fmlAux);
      }
/* Control existencia Contrato Unico [FIN] */
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
