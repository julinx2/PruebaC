#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <cliEnrolamiento.h>
#include <GestionDeErrores.h>


long CliIncrementarIntentosFallidos(tCliCLAVEDEACCESOINTERNET claveAcceso)
{
   long sts=0;

   sts = SqlCliIncrementarIntentosFallidosClaveInternet(&claveAcceso);
   if (sts != SQL_SUCCESS)
      return (sts);

   if (claveAcceso.intentosFallidos >= CLI_CANTIDAD_MAXIMA_ACCESO_FALLIDOS)
   {
      sts = SqlCliBloquearClaveDeAccesoInternet(&claveAcceso);
      if (sts != SQL_SUCCESS)
         return (sts);
   }

   return(SQL_SUCCESS);
}


long CliPosicionPreguntaSecretaRandom(int *oIndice, char *oArreglo)
{
   short   i,j;
   int     largoArr, numeroRandom=0;
   char    chArreglo[4+1];
   char    chIndice[1+1];
   

  numeroRandom = (random()%4);

  largoArr = (strlen(oArreglo))-1;

  if (numeroRandom <= largoArr) 
  { 
     strncpy(chIndice, oArreglo+numeroRandom,1);
     chIndice[1]='\0'; 

     *oIndice = atoi(chIndice);

     j=0;
     for(i = 0;   i <= largoArr;i++ )
     {
        if (i != numeroRandom)
        {
           chArreglo[j] = oArreglo[i]; 
           j++;
        }
     }
     chArreglo[j] = '\0';
     strcpy(oArreglo, chArreglo);
  }
  else
  {
     strncpy(chIndice, oArreglo,1);
     chIndice[1]='\0'; 
     *oIndice = atoi(chIndice);
  }

   return(SQL_SUCCESS);
}


/************************************************************************/
/* Objetivo: Funcion encargada de normalizar los datos de la activación */
/*           una vez que el cliente se ha reenrolado (firma mandato)    */
/* Autor   : Boris Contreras Mac-lean             Fecha: 30-03-2005     */
/************************************************************************/
long CliReenrolarClientePorMandatoEnrolamiento(tCliCLIENTESINDOCUMENTO clienteSinDocumento, short porAplicacionNueva, tCliDOCUMENTACIONFIRMADA *documentacionFirmada, short *enCampanaReenrolamiento, char *mensaje)
{
   long    sts;
   tCliUSUARIOACTIVACIONENROL         usuarioActivacionEnrol;
   tCliENROLAMIENTOEMPRESARELACIONADA enrolamientoEmpresaRelacionada;


    *enCampanaReenrolamiento = 0;

   /*----- Verifico si el cliente está en campaña de reenrolamiento -----*/
   sts = SqlCliRecuperarClienteSinDocumento(&clienteSinDocumento);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      strcpy(mensaje, "Error al recuperar información sobre campaña de reenrolamiento.");
      return (SRV_CRITICAL_ERROR);
   }

   /*----- Si el cliente se encuentra en campaña de reenrolamiento -----*/
   if (sts == SQL_SUCCESS)
   {
      *enCampanaReenrolamiento = 1;

      /*----- Se ingresa la relación enrolamiento - empresaRelacionada -----*/
      enrolamientoEmpresaRelacionada.rutCliente = clienteSinDocumento.rutCliente;
      enrolamientoEmpresaRelacionada.empresaRelacionada = CLI_RUT_BANCO_FALABELLA;
      enrolamientoEmpresaRelacionada.nroBloqueosActivos = 0;
      enrolamientoEmpresaRelacionada.estado = CLI_ENROLAMIENTO_ACTIVADO;

      sts = SqlCliInsertarEnrolamientoEmpresaRelacionada(&enrolamientoEmpresaRelacionada);
      if (sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
      {
         strcpy(mensaje, "No se pudo insertar la activación del cliente por campaña de reenrolamiento.");
         return (SRV_CRITICAL_ERROR);
      }

      /*----- Se actualiza la fecha de ingreso del documento -----*/
      sts = SqlCliActualizarClienteSinDocumento(clienteSinDocumento);
      if (sts != SQL_SUCCESS)
      {
         strcpy(mensaje, "Error al actualizar información sobre campaña de reenrolamiento.");
         return (SRV_CRITICAL_ERROR);
      }

      /*----- Se mantiene el flag que indica si se cancelo o no el gasto notarial -----*/
      strcpy(documentacionFirmada->seCobroGastoNotarial, clienteSinDocumento.seCobroGastoNotarial);


      /*----- Se actualiza la tabla cliente campaña, para eliminarlo de los mensajes VB -----*/
      sts = SqlCliActualizarClienteCampana(clienteSinDocumento);
      if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
      {
         strcpy(mensaje, "Error al actualizar información sobre campaña del cliente.");
         return (SRV_CRITICAL_ERROR);
      }

      /*----- El ingreso del registro en la tabla DocumentacionFirmada se hace afuera -----*/


      if (porAplicacionNueva)
      {
         /*----- Se ingresa el usuario que activa el enrolamiento, si la activacion es por la aplicacion Gothic -----*/
         usuarioActivacionEnrol.rutCliente = clienteSinDocumento.rutCliente;
         usuarioActivacionEnrol.empresaRelacionada = CLI_RUT_BANCO_FALABELLA;
         usuarioActivacionEnrol.usuario = 0;
         usuarioActivacionEnrol.empresaExterna = 0;

         sts = SqlCliInsertarUsuarioActivacionEnrolamiento(&usuarioActivacionEnrol);
         if (sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
         {
            strcpy(mensaje, "Error al ingresar usuario activacion de reenrolamiento, por campaña.");
            return (SRV_CRITICAL_ERROR);
         }
      }
      else
      {

		 /*----- Ingreso de datos en esquema de "Enrolamiento" (Gothic) ----*/
         sts = SqlCliInsertarRequisitoEmpresaClienteGothic(clienteSinDocumento.rutCliente, enrolamientoEmpresaRelacionada.empresaRelacionada, CLI_GOTHIC_MANDATO_ENROLAMIENTO);

         if (sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
         {
            strcpy(mensaje, "No se pudo insertar el mandato por enrolamiento en el esquema de enrolamiento (Gothic).");
            return (SRV_CRITICAL_ERROR);
         }
      }
   }


   return(SRV_SUCCESS);
}

/*** JAS BYPASS [INI] ***/
long CliReenrolarClientePorMandatoEnrolamientoGothic(tCliCLIENTESINDOCUMENTO clienteSinDocumento, 
                                                     short porAplicacionNueva, 
                                                     tCliDOCUMENTACIONFIRMADA *documentacionFirmada, 
                                                     short *enCampanaReenrolamiento, char *mensaje)
{
   long    sts;
   tCliUSUARIOACTIVACIONENROL         usuarioActivacionEnrol;
   tCliENROLAMIENTOEMPRESARELACIONADA enrolamientoEmpresaRelacionada;


    *enCampanaReenrolamiento = 0;

   /*----- Verifico si el cliente está en campaña de reenrolamiento -----*/
   sts = SqlCliRecuperarClienteSinDocumento(&clienteSinDocumento);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      strcpy(mensaje, "Error al recuperar información sobre campaña de reenrolamiento.");
      return (SRV_CRITICAL_ERROR);
   }

   /*----- Si el cliente se encuentra en campaña de reenrolamiento -----*/
   if (sts == SQL_SUCCESS)
   {
      *enCampanaReenrolamiento = 1;

      /*----- Se actualiza la fecha de ingreso del documento -----*/
      sts = SqlCliActualizarClienteSinDocumento(clienteSinDocumento);
      if (sts != SQL_SUCCESS)
      {
         strcpy(mensaje, "Error al actualizar información sobre campaña de reenrolamiento.");
         return (SRV_CRITICAL_ERROR);
      }

      /*----- Se mantiene el flag que indica si se cancelo o no el gasto notarial -----*/
      strcpy(documentacionFirmada->seCobroGastoNotarial, clienteSinDocumento.seCobroGastoNotarial);


      /*----- Se actualiza la tabla cliente campaña, para eliminarlo de los mensajes VB -----*/
      sts = SqlCliActualizarClienteCampana(clienteSinDocumento);
      if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
      {
         strcpy(mensaje, "Error al actualizar información sobre campaña del cliente.");
         return (SRV_CRITICAL_ERROR);
      }

      /*----- El ingreso del registro en la tabla DocumentacionFirmada se hace afuera -----*/


      if (!porAplicacionNueva)
      {
         /*----- Ingreso de datos en esquema de "Enrolamiento" (Gothic) ----*/
         sts = SqlCliInsertarRequisitoEmpresaClienteGothic(clienteSinDocumento.rutCliente, 
                                                           enrolamientoEmpresaRelacionada.empresaRelacionada, 
                                                           CLI_GOTHIC_MANDATO_ENROLAMIENTO);

         if (sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
         {
            strcpy(mensaje, "No se pudo insertar el mandato por enrolamiento en el esquema de enrolamiento (Gothic).");
            return (SRV_CRITICAL_ERROR);
         }
      }
   }


   return(SRV_SUCCESS);
}
/*** JAS BYPASS [FIN] ***/

