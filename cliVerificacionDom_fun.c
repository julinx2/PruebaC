
/****F* cliente/servidores/fuentes
  *  NAME
  *    cliVerificacionDom_fun.c
  *  AUTHOR
  *    Silvia López Salas 
  *  CREATION DATE
  *    01/01/2009
  *  DESCRIPTION
  *    Funciones para cliVerificacionDom.c
  *  PARAMETERS
  *		DeterminarComposicionInstitucional
  *  RETURN VALUE
  *     
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <atmi.h>           /* TUXEDO Header File API's */
#include <userlog.h>        /* TUXEDO Header File */
#include <fml32.h>            /* TUXEDO FML Support */

#include <olist.h>
#include <omegafec2.h>
#include <GestionDeErrores.h>

#include <hermesFML.h>
#include <sgtFML.h>

#include <cliVerificacionDom_cons.h>
#include <cliVerificacionDom.h>
#include <cliVerificacionDom_fun.h>
#include <cliVerificacionDom_sql.h>

int FunComponeFechaHoraSistema(char *fechaSys)
{
  long horaL;
  char h[3],m[3],s[3],sHora[7],sFecha[9];

  horaL = f_horaComoLong();
  sprintf(sHora,"%06li",horaL);

  strncpy(h,sHora,2);
  strncpy(m,sHora+2,2);
  strncpy(s,sHora+4,2);

  h[2] = '\0';
  m[2] = '\0';
  s[2] = '\0';

  strcat(fechaSys," ");

  strcat(fechaSys,h);

  *(fechaSys+11) = ':';
  *(fechaSys+12) = '\0';

  strcat(fechaSys,m);
  *(fechaSys+14) = ':';
  *(fechaSys+15) = '\0';

  strcat(fechaSys,s);
  return(0);
}

long TUXRecuperarFechaProcesoYCalendario(char *oFechaProceso, char *oFechaCalendario, char *oMensajeError)
{
   FBFR32    *fml;
   char    mensaje[500];
   long    largo=0, sts=0;


   if (( fml = (FBFR32 *)tpalloc("FML32",NULL,1024)) == NULL)
   {
     userlog("Error de sistema\n");
     return -1;
   }

   if (( sts = tpcall("SgtRecParametro", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN)) == -1)
   {
      Fget32( fml, HRM_MENSAJE_SERVICIO,  0, mensaje, 0);
      userlog("empext (Rec fechas SGT) - MENSAJE ERROR: (%s)", mensaje);

      strcpy(oMensajeError,mensaje);

      tpfree((char *)fml);
      return -1;
   }

   Fget32(fml, SGT_FECHA, 0, oFechaProceso, 0);
   Fget32(fml, SGT_FECHA_CALENDARIO, 0, oFechaCalendario, 0);

   tpfree((char *)fml);
   return (0);
}



