/************************************************************************/
/*                             CLIENTE                                  */
/*                      SERVIDOR:CliVerificacionDom                     */
/*                                                                      */
/************************************************************************/
/* Omega Ingenieria de Software Ltda                                    */
/* Fecha de Desarrollo  : Febrero 2009                                  */
/************************************************************************/
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <atmi.h>           /* TUXEDO Header File API's */
#include <userlog.h>        /* TUXEDO Header File */
#include <fml32.h>            /* TUXEDO FML Support */

#include <olist.h>
#include <omegafml32.h>
#include <servidor.h>
#include <GestionDeErrores.h>
#include <buffer.h>

#include <hermesFML.h>

#include <cliVerificacionDomFML.h>
#include <cliVerificacionDom_cons.h>
#include <cliVerificacionDom.h>
#include <cliVerificacionDom_fun.h>
#include <cliVerificacionDom_sql.h>

char gFechaProceso[8+1];
char gFechaCalendario[8+1];

tpsvrinit(int argc, char *argv[])
{
   long  sts;

   if (tpopen() == -1)
   {
      userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al conectarse con el Administrador de Base de Datos");
      exit(1);
   }

   userlog("El Servidor de Cliente: \"verificacionDomicilio\", ha iniciado su ejecucion satisfactoriamente...");
   return 0;
}


void tpsvrdone()
{
   if (tpclose() == -1)
   {
      switch(tperrno)
      {
         case TPERMERR:
            userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al desconectarse del Administrador de Base de Datos para mas informacion consultar al manejador especifico");
            break;

         case TPEPROTO:
            userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al desconectarse del Administrador de Base de Datos debido a un problema de contexto del close()");
            break;

         case TPESYSTEM:
            userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al desconectarse del Administrador de Base de Datos debido a un problema con Tuxedo-System/T");
            break;

         case TPEOS:
            userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al desconectarse del Administrador de Base de Datos debido a un error del Sistema Operativo");
            break;

         default:
            userlog("El Servidor de Cliente: \"verificacionDomicilio\", fallo al desconectarse del Administrador de Base de Datos debido a un error de excepcion en el sistema");
            break;
      }
	  return ;
   }
   userlog("El Servidor de Cliente: \"verificacionDomicilio\", ha finalizado su ejecucion...");
   return ;
}

/******************************************************************************/
/* Objetivo: Recuperar Detalle consulta solicitud verificación de domicilio.  */
/* Autor   : Miriam Flores                    Fecha: 27 abril 2009            */
/* Modifica: Silvia López Salas               Fecha: 11 Noviembre 2009        */
/******************************************************************************/
void CliRecDetVerDom(TPSVCINFO *rqst)
{
   FBFR32          *fml;
   int             transaccionGlobal;
   long            sts;
   tCliVFDEvento   eventoVerificacionDomicilio;
   tCliVFDdetalle  verificacionDomicilio;
   tCliVFDEstado   estadoVerificacionDomicilio;

  
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   memset((char *)&eventoVerificacionDomicilio,0,sizeof(tCliVFDEvento));

   Fget32(fml, VFD_RUT, 0, (char *)&verificacionDomicilio.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);
   
   sts = SqlVfdRecuperaVerificacionDomicilio(&verificacionDomicilio);
   if (sts != SQL_SUCCESS)
   {
      if  (sts == SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se encontro Informacion del cliente", 0);
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error al recuperar verificacion de domicilio del cliente", 0);
      }
      userlog("%s: Error en funcion SqlVfdRecuperaVerificacionDomicilio.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlVfdRecUnEventoVerDom(verificacionDomicilio.rut,verificacionDomicilio.estado, &eventoVerificacionDomicilio);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no registra evento de verificación de domicilio.", 0);
      userlog("%s: Error en funcion SqlVfdRecUnEventoVerDom", rqst->name);
      TRX_ABORT(transaccionGlobal);;
   }

   Fadd32(fml, VFD_RUT, (char *)&verificacionDomicilio.rut, 0); 
   Fadd32(fml, VFD_DIRECCION, verificacionDomicilio.direccion, 0);
   Fadd32(fml, VFD_NOMBRE_COMUNA, eventoVerificacionDomicilio.nombreComuna, 0);
   Fadd32(fml, VFD_NOMBRE_CIUDAD, eventoVerificacionDomicilio.nombreCiudad, 0);
   Fadd32(fml, VFD_DESCRIPCION, verificacionDomicilio.descripcion,0);
   Fadd32(fml, VFD_USUARIO, (char *)&eventoVerificacionDomicilio.usuario, 0); 
   Fadd32(fml, VFD_SUCURSAL, (char *)&eventoVerificacionDomicilio.sucursal,0);
   Fadd32(fml, VFD_FECHA_SOLICITUD, eventoVerificacionDomicilio.fechaSolicitud, 0); 
   Fadd32(fml, VFD_FECHA_INFORME, eventoVerificacionDomicilio.fechaInforme, 0);   
   Fadd32(fml, VFD_EXISTE_DIRECCION, eventoVerificacionDomicilio.existeDireccion, 0); 
   Fadd32(fml, VFD_RESTRICCION, eventoVerificacionDomicilio.restriccion, 0); 
   Fadd32(fml, VFD_OBSERVACIONES, eventoVerificacionDomicilio.observacionesGenerales, 0);
   Fadd32(fml, VFD_CUENTA_CON_FORMATO, (char *)&verificacionDomicilio.numeroOperacion, 0);
   Fadd32(fml, VFD_REFERENCIA, eventoVerificacionDomicilio.referencia, 0);
   Fadd32(fml, VFD_MOTIVO_MODIFICACION, (char *)&eventoVerificacionDomicilio.motivoModificacion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************************************************************************/
/* Objetivo: Recupera el estado de verificacion de domicilio de un cliente  */
/* Autor   : Miriam Flores                      Fecha: 23 abril 2009        */
/*         :                                                                */
/* Modifica: Silvia López Salas                 Fecha: 09 noviembre 2009    */
/****************************************************************************/
void CliRecEstCliVD(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rutCliente;
   short estado;
   char descripcion[30+1];

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   
   Fget32(fml, VFD_RUT, 0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   sts = SqlVfdRecuperarVerificarSolicitudDomicilio(rutCliente, &estado);
   if (sts != SQL_SUCCESS)
   {
      if  (sts == SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se ha verificado domicilio del cliente", 0);
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error al recuperar verificacion de domicilio del cliente", 0);
         userlog("%s: Error en funcion SqlVfdRecuperarVerificarSolicitudDomicilio.", rqst->name);
      }
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlVfdRecuperaDescVerificacionDomicilio (estado,descripcion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en la recuperacion de la descripción del estado", 0);
      userlog("%s: Error en funcion SqlVfdRecuperaDescVerificacionDomicilio", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   Fadd32(fml, VFD_ESTADO, (char *)&estado, 0);
   Fadd32(fml, VFD_DESCRIPCION, descripcion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*****************************************************************************/
/* Objetivo: Recuperar todos los Documentos de Verificacion de Domicilio     */
/* Autor   : Miriam Flores                                 Fecha: 24-04-2009 */
/* Modifica:                                               Fecha:            */
/*****************************************************************************/
void CliRecTipoDocVD(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliVFDDocumentos *docVerificacionDomicilio;
   Q_HANDLE *lista;
   int totalRegistros=0, i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(20,20)) == NULL)
   {
      sts = SQL_MEMORY;
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlVfdRecuperarDocumentosVerificacionDomicilio(lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen documentos de Verificacion Domicilio.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar los documentos de Verificaicon Domicilio.", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3*totalRegistros,
             sizeof(tCliVFDDocumentos)* totalRegistros));

   for (i=0; i < lista->count ; i++)
   {
      docVerificacionDomicilio = (tCliVFDDocumentos *)QGetItem(lista,i);

      Fadd32(fml, VFD_CODIGO, (char *)&docVerificacionDomicilio->codigo, 0);
      Fadd32(fml, VFD_DESCRIPCION, docVerificacionDomicilio->descripcion, 0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Inserta Verificacion Domicilio                              */
/* Autor   : Miriam Flores                 Fecha: 28 abril 2009          */
/*         :                                                             */
/* Modifica:                               Fecha:                        */
/*************************************************************************/
void CliInsVerDom(TPSVCINFO *rqst)
{
   FBFR32                 *fml;
   int                    transaccionGlobal;
   long                   sts;
   tCliVFDdetalle         verificacionDomicilio;
   tCliVFDEvento          eventoVerificacionDomicilio;
   tCliVFDCliente         cliente;
   tCliVFDClienteEmpresa  clienteEmpresa;
   tCliVFDDireccion       direccion;
   char                   fechaProceso[DIM_FECHA], fechaCalendario[14+1];
   char                   fecha[14+1],error[100];
   long                   rutEmpExt;
   char                   descComuna[50+1];
   char                   descCiudad[50+1];
   short                  estado;
   int                    codprod;
   char                   prod[1+1];
   char                   nrocuenta[12+1];
   double                 auxnrocuenta;
   short                  auxEstado=0;
   short                  tipoVerifDom;
   short                  sucursal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   memset((char *)&eventoVerificacionDomicilio,0,sizeof(tCliVFDEvento));

   Fget32(fml, VFD_RUT, 0, (char *)&verificacionDomicilio.rut, 0);
   Fget32(fml, VFD_CUENTA_VERDOM, 0, (char *)&verificacionDomicilio.numeroCuentaCte, 0);
   Fget32(fml, VFD_TIPO_VERIF_DOM, 0, (char *)&tipoVerifDom,0); 
   Fget32(fml, VFD_USUARIO, 0, (char *)&eventoVerificacionDomicilio.usuario, 0);   
   Fget32(fml, VFD_SUCURSAL, 0, (char *)&sucursal,0);
   Fget32(fml, VFD_REFERENCIA, 0, eventoVerificacionDomicilio.referencia,0);
   Fget32(fml, VFD_MOTIVO_MODIFICACION, 0, (char *)&eventoVerificacionDomicilio.motivoModificacion,0);

   
   if (Foccur32(fml, VFD_ESTADO) > 0)
      Fget32(fml, VFD_ESTADO, 0, (char *)&auxEstado, 0);
   
   auxnrocuenta=verificacionDomicilio.numeroCuentaCte;
   /*** Se obtiene el codigo producto a partir del número de cuenta ***/
   sprintf(nrocuenta,"%.0f",auxnrocuenta);
   strncpy(prod,nrocuenta,1);
   prod[1]='\0';
   codprod=atoi(prod);

   verificacionDomicilio.producto=codprod;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
   {
       if (tpbegin(TIME_OUT,0L) == -1)
       {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
           tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
       }
   }

   sts = TUXRecuperarFechaProcesoYCalendario(fechaProceso, fechaCalendario, error);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, VFD_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion TUXRecuperarFechaProcesoYCalendario.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   sprintf(fecha,"%14s", fechaCalendario);
   fecha[14]='\0';

   /****Recuperar Cliente******/
   cliente.rut = verificacionDomicilio.rut;
   sts = SqlCliRecuperarCliente(&cliente);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se puede insertar verificación de domicilio. Persona no existe como cliente.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error al recuperar cliente.", 0);
         userlog("%s: Error en funcion SqlCliRecuperarCliente.", rqst->name);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   direccion.rutCliente = verificacionDomicilio.rut;
   direccion.tipoDireccion = CLI_VFD_DIRECCION_PARTICULAR;
   sts = SqlVfdRecuperarUnaDireccion(&direccion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Direccion no existe", 0);
      userlog("%s: Error en funcion SqlVfdRecuperarUnaDireccion.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   strcpy(verificacionDomicilio.direccion, direccion.calleNumero);
   if (strlen(direccion.departamento) > 0 && strcmp(direccion.departamento,"        ") != 0)
   {
      strcat(verificacionDomicilio.direccion, "DEPARTAMENTO");
      strcat(verificacionDomicilio.direccion, direccion.departamento);
   }
   else
   {
      strcat(verificacionDomicilio.direccion, " ");
   }
   strcat(verificacionDomicilio.direccion, direccion.villaPoblacion);
   sprintf(verificacionDomicilio.numeroOperacion,"%.0f",verificacionDomicilio.numeroCuentaCte);


   if (auxEstado == 0) /*cliente sin verificacion */
   {
      if(tipoVerifDom == CLI_VFD_ACREDITACION_CON_DOC){
         estado = CLI_VFD_ESTADO_ACEPTADA_INTERNA;
         strcpy(eventoVerificacionDomicilio.fechaSolicitud, fechaProceso);
         strcpy(eventoVerificacionDomicilio.fechaInforme, fechaProceso);
      }else{
         estado = CLI_VFD_ESTADO_PARA_ENVIO;
         strcpy(eventoVerificacionDomicilio.fechaSolicitud, fechaProceso);
      }
   }
   else
   {
      estado = auxEstado;
   }

   verificacionDomicilio.estado = estado;

   if (auxEstado == 0) /*cliente sin verificacion */
   {
      sts = SqlVfdInsertaVerifDomicilio(&verificacionDomicilio);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Insertar Cliente en tabla Verificación Domicilio ", 0);
         userlog("%s: Error en funcion SqlVfdInsertaVerifDomicilio.", rqst->name);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
     
   }else{ /* cliente con verificacion previa */
      
      sts = SqlVfdModificaVerifDomicilio(&verificacionDomicilio);
      if (sts != SQL_SUCCESS)
      {   
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Modificar Cliente en Verificación Domicilio.", 0);
         userlog("Error en funcion SqlVfdModificaVerifDomicilio.");
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   sts = SqlVfdRecuperaDesComYCiu(direccion.comuna,descComuna,descCiudad);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar la descripcion de la comuna y ciudad. ", 0);
      userlog("%s: Error en funcion SqlVfdRecuperaDesComYCiu.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   eventoVerificacionDomicilio.rut = verificacionDomicilio.rut;
   strcpy(eventoVerificacionDomicilio.fechaHoraEvento,fecha);
   eventoVerificacionDomicilio.estado = estado;
   eventoVerificacionDomicilio.sucursal = sucursal;
   eventoVerificacionDomicilio.producto = codprod;
   strcpy(eventoVerificacionDomicilio.numeroOperacion,verificacionDomicilio.numeroOperacion);
   strcpy(eventoVerificacionDomicilio.direccion,verificacionDomicilio.direccion);
   strcpy(eventoVerificacionDomicilio.nombreComuna,descComuna);
   strcpy(eventoVerificacionDomicilio.nombreCiudad,descCiudad);
   
   sts = SqlVfdInsertaEventoVerDomDoc(&eventoVerificacionDomicilio);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar un Evento de Verificacion de Domicilio. ", 0);
      userlog("%s: Error en funcion SqlVfdInsertaEventoVerDomDoc.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}


