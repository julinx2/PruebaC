
/****F* cliente/servidores/fuentes
  *  NAME
  *    cliente_fun.c
  *  AUTHOR
  *    Pilar Aedo Pezoa
  *  CREATION DATE
  *    01/08/2007
  *  DESCRIPTION
  *    Funciones para cliente.c
  *  PARAMETERS
  *		DeterminarComposicionInstitucional
  *  RETURN VALUE
  *     *ptrOTipoEstrato
  *  MODIFICATION HISTORY
  *		19-11-2007 Modificaciones del Req.00636
  *		03-09-2014 Se inicializan campos de estructura en rutina
  *                        LlenarRegistroActividad, LlenarRegistroRegMod
  *                        dado que en caso de no recibir estos campos
  *                        en la invocacion podria provoca problema
  *                        en sentencia SQL. (Marcelo Roman P.)
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
#include <fml32.h>          /* TUXEDO FML Support */

#include <olist.h>
#include <omegafec2.h>
#include <GestionDeErrores.h>

#include <clienteFML.h>
#include <cliClienteConsolidadoFML.h>
#include <creditoFML.h>
#include <evaparamFML.h>
#include <hermesFML.h>
#include <sgtFML.h>
#include <tarjetaCreditoFML.h> 

#include <tarjetaCredito.h>

#include <cliente.h>
#include <cliente_fun.h>
#include <cliente_sql.h>

long ObtenerRutIvr(long rutEntrada, char *rutSalida)
{
   char rutChar[CLI_LARGO_RUT + 1];
   char rutCompuesto[CLI_LARGO_BLOCK + 1];
   short i=0;
   short j=0;

   memset(rutChar, 0, sizeof(rutChar));
   memset(rutCompuesto, 0, sizeof(rutCompuesto));


   for(i=0; i<=CLI_LARGO_RUT; i++)
      rutChar[i] = 0;

   sprintf(rutChar, "%li", rutEntrada);

   for(i=0,j=0; i<CLI_LARGO_BLOCK; i++,j++)
   {
      if(rutChar[j] == 0)
         j = 0;

      rutCompuesto[i] = rutChar[j];
   }
   rutCompuesto[CLI_LARGO_BLOCK] = 0;

   sprintf(rutSalida, "%s", rutCompuesto );


   return(SRV_SUCCESS);
}


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

void LlenarRegistroCliente(FBFR32 *fml, tCliCLIENTE *cliente, 
                        tCliINFOADM *informacion, int flag)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&cliente->rut, 0);
   Fget32(fml, CLI_DVX, 0, cliente->dv, 0);
   Fget32(fml, CLI_NOMBRES, 0, cliente->nombres, 0);
   Fget32(fml, CLI_APELLIDO_PATERNO, 0, cliente->apellidoPaterno, 0);
   Fget32(fml, CLI_APELLIDO_MATERNO, 0, cliente->apellidoMaterno, 0);
   Fget32(fml, CLI_FECHA_NACIMIENTO, 0, cliente->fechaNacimiento, 0);
   Fget32(fml, CLI_SEXOX, 0, (char *)&cliente->sexo, 0);
   Fget32(fml, CLI_NACIONALIDADX, 0, (char *)&cliente->nacionalidad, 0);
   Fget32(fml, CLI_ESTADO_CIVIL, 0, (char *)&cliente->estadoCivil, 0);
   Fget32(fml,CLI_REGIMEN_MATRIMONIAL,0,
          (char *)&cliente->regimenMatrimonial,0);

   if (cliente->estadoCivil != CLI_EST_CIVIL_CASADO)
      strcpy(cliente->fechaMatrimonio,CLI_FECHA_NULA);
   else
      Fget32(fml,CLI_FECHA_MATRIMONIO,0,cliente->fechaMatrimonio,0);

   Fget32(fml, CLI_NUM_CARGAS_FAMILIARES, 0, 
        (char *)&cliente->numCargasFamiliares, 0);
   Fget32(fml, CLI_NIVEL_EDUCACIONAL, 0, 
           (char *)&cliente->nivelEducacional, 0);
   Fget32(fml, CLI_PROFESION, 0, (char *)&cliente->profesion, 0);
   Fget32(fml, CLI_ESTADO, 0, (char *)&cliente->estado, 0);

   /* Se incluye dirección electrónica */
   if (Foccur32(fml, CLI_EMAIL) > 0)
      Fget32(fml, CLI_EMAIL, 0 , cliente->direccionElectronica, 0);
   else
     strcpy(cliente->direccionElectronica, " ");

   /* 12-09-2013 se incluye codigo Pais*/
   if (Foccur32(fml, CLI_CODIGO_PAIS) > 0)
      Fget32(fml, CLI_CODIGO_PAIS, 0, cliente->codigoPais, 0);
   else
      strcpy(cliente->codigoPais, "");

   if (cliente->nacionalidad == 1)
       strcpy(cliente->codigoPais, "160");

/* Implantación de Normativa FATCA PJVA INI */
   if (Foccur32(fml, CLI_CODIGO_PAIS_RESIDENCIA) > 0)
      Fget32(fml, CLI_CODIGO_PAIS_RESIDENCIA, 0, cliente->codigoPaisResidencia, 0);
   else
      strcpy(cliente->codigoPaisResidencia, "");
	  
   if (Foccur32(fml, CLI_CODIGO_PAIS_NACIMIENTO) > 0)
      Fget32(fml, CLI_CODIGO_PAIS_NACIMIENTO, 0, cliente->codigoPaisNacimiento, 0);
   else
      strcpy(cliente->codigoPaisNacimiento, "");
/* Implantación de Normativa FATCA PJVA FIN */

   /*flag = 1 para crear; flag = 2 para modificar*/ 
   /*los datos de info no se deben modificar*/ 
   if (flag == 1)
   {
     Fget32(fml, CLI_USUARIO, 0, (char *)&informacion->usuario, 0);
     Fget32(fml, CLI_SUCURSAL, 0, (char *)&informacion->sucursal, 0);
     Fget32(fml, CLI_FECHA, 0, informacion->fechaAdmision, 0);
     informacion->rutCliente = cliente->rut;
   }

   
   if (Foccur32(fml, CLI_CLIENTE_CRS) > 0)
   Fget32(fml, CLI_CLIENTE_CRS, 0, cliente->clienteCrs, 0);  
   else
      cliente->clienteCrs[0]=0;

}

void ObtieneRegistroCliente(FBFR32 *fml, tCliCLIENTE *cliente, tCliINFOADM *informacion)
{
   Fadd32(fml, CLI_DVX, cliente->dv, 0);
   Fadd32(fml, CLI_NOMBRES, cliente->nombres, 0);
   Fadd32(fml, CLI_APELLIDO_PATERNO, cliente->apellidoPaterno, 0);
   Fadd32(fml, CLI_APELLIDO_MATERNO, cliente->apellidoMaterno, 0);
   Fadd32(fml, CLI_FECHA_NACIMIENTO, cliente->fechaNacimiento, 0);
   Fadd32(fml, CLI_SEXOX, (char *)&cliente->sexo, 0);
   Fadd32(fml, CLI_NACIONALIDADX, (char *)&cliente->nacionalidad, 0);
   Fadd32(fml, CLI_ESTADO_CIVIL, (char *)&cliente->estadoCivil, 0);
   Fadd32(fml, CLI_REGIMEN_MATRIMONIAL, 
             (char *)&cliente->regimenMatrimonial, 0);
   Fadd32(fml, CLI_FECHA_MATRIMONIO, cliente->fechaMatrimonio, 0);
   Fadd32(fml, CLI_NUM_CARGAS_FAMILIARES, 
             (char *)&cliente->numCargasFamiliares, 0);
   Fadd32(fml, CLI_NIVEL_EDUCACIONAL, 
             (char *)&cliente->nivelEducacional, 0);
   Fadd32(fml, CLI_PROFESION, (char *)&cliente->profesion, 0);
   Fadd32(fml, CLI_ESTADO, (char *)&cliente->estado, 0);
   Fadd32(fml, CLI_USUARIO, (char *)&informacion->usuario, 0);
   Fadd32(fml, CLI_SUCURSAL, (char *)&informacion->sucursal, 0);
   Fadd32(fml, CLI_FECHA, informacion->fechaAdmision, 0);
   Fadd32(fml, CLI_EMAIL ,  cliente->direccionElectronica , 0);

   Fadd32(fml, CLI_FLAG_PEP, (char *)&cliente->pep, 0);
   Fadd32(fml, CLI_CODIGO_PAIS, cliente->codigoPais, 0);
   
   /* Implantación de Normativa FATCA PJVA INI */
   Fadd32(fml, CLI_CODIGO_PAIS_RESIDENCIA, cliente->codigoPaisResidencia, 0);
   Fadd32(fml, CLI_CODIGO_PAIS_NACIMIENTO, cliente->codigoPaisNacimiento, 0);
   /* Implantación de Normativa FATCA PJVA FIN */

   Fadd32(fml, CLI_MENSAJE_CLIENTE_PEP, cliente->mensajeClientePEP, 0); /* Proyecto Cambio Mensaje Cliente PEP */
   Fadd32(fml, CLI_ES_CLIENTE_FACTA, (char *)&cliente->esClienteFatcaUS, 0);
   
   Fadd32(fml, CLI_DESCRIPCION, cliente->tipoCliente, 0);  
   Fadd32(fml, CLI_CLIENTE_CRS, cliente->clienteCrs, 0);

}

void LlenarRegistroConyuge(FBFR32 *fml, tCliCONYUGE *conyuge)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&conyuge->rutCliente, 0);
   Fget32(fml, CLI_RUT_CONYUGE, 0, (char *)&conyuge->rutConyuge, 0);

   Fget32(fml, CLI_DVX, 0, conyuge->dv, 0);
   if (strlen(conyuge->dv) == 0) strcpy(conyuge->dv," ");
   Fget32(fml, CLI_NOMBRES, 0, conyuge->nombres, 0);
   Fget32(fml, CLI_APELLIDO_PATERNO, 0, conyuge->apellidoPaterno, 0);
   Fget32(fml, CLI_APELLIDO_MATERNO, 0, conyuge->apellidoMaterno, 0);
   Fget32(fml, CLI_PROFESION, 0, (char *)&conyuge->profesion, 0);
   Fget32(fml, CLI_TRABAJA, 0, conyuge->trabaja, 0);

   if (strncmp(conyuge->trabaja,CLI_SI,1) == 0)
   {
     Fget32(fml, CLI_NOMBRE_EMPRESA, 0, conyuge->nombreEmpresa, 0);
     Fget32(fml, CLI_RENTA_LIQUIDA, 0, (char *)&conyuge->rentaLiquida, 0);
     Fget32(fml, CLI_ANO_INGRESO, 0,(char *)& conyuge->anoIngreso, 0);
     Fget32(fml, CLI_MES_INGRESO, 0,(char *)& conyuge->mesIngreso, 0);
     Fget32(fml, CLI_CARGO_CONYUGE, 0,(char *)& conyuge->cargo, 0);
   }
   else
   {
     strcpy(conyuge->nombreEmpresa,CLI_UN_BLANCO);
     conyuge->rentaLiquida = CLI_VALOR_POR_DEFECTO_RENTA;
     conyuge->anoIngreso   = CLI_VALOR_POR_DEFECTO;
     conyuge->mesIngreso   = CLI_VALOR_POR_DEFECTO;
     conyuge->cargo        = CLI_VALOR_POR_DEFECTO;
   }

}

void ObtieneRegistroConyuge(FBFR32 *fml, tCliCONYUGE *conyuge)
{
   Fadd32(fml, CLI_RUT_CONYUGE, (char *)&conyuge->rutConyuge, 0);
   Fadd32(fml, CLI_DVX, conyuge->dv, 0);
   Fadd32(fml, CLI_NOMBRES, conyuge->nombres, 0);
   Fadd32(fml, CLI_APELLIDO_PATERNO, conyuge->apellidoPaterno, 0);
   Fadd32(fml, CLI_APELLIDO_MATERNO, conyuge->apellidoMaterno, 0);
   Fadd32(fml, CLI_PROFESION, (char *)&conyuge->profesion, 0);
   Fadd32(fml, CLI_TRABAJA, conyuge->trabaja, 0);
   Fadd32(fml, CLI_NOMBRE_EMPRESA, conyuge->nombreEmpresa, 0);
   Fadd32(fml, CLI_RENTA_LIQUIDA, (char *)&conyuge->rentaLiquida, 0);
   Fadd32(fml, CLI_ANO_INGRESO,(char *)& conyuge->anoIngreso, 0);
   Fadd32(fml, CLI_MES_INGRESO,(char *)& conyuge->mesIngreso, 0);
   Fadd32(fml, CLI_CARGO_CONYUGE,(char *)& conyuge->cargo, 0);
}
 
void LlenarRegistroActividad(FBFR32 *fml, tCliACTIVIDAD *actividad)
{
   char nombreEmpresa[80+1];

   Fget32(fml, CLI_RUTX, 0, (char *)&actividad->rutCliente, 0);

/** Inicio Modificacion 03-09-2014 (MARP) **/
   if (Fget32(fml, CLI_TIPO_EMPLEO, 0, (char *)&actividad->tipoEmpleo, 0) == -1)
      actividad->tipoEmpleo = 0;

   if (Fget32(fml, CLI_TIPO_RENTA, 0, (char *)&actividad->tipoRenta, 0) == -1)
      actividad->tipoEmpleo = 0;

   if (Fget32(fml, CLI_ACTIVIDAD, 0, (char *)&actividad->actividad, 0) == -1)
      actividad->actividad = 0;

   if (Fget32(fml, CLI_RUT_EMPRESA, 0, (char *)&actividad->rutEmpresa, 0) == -1)
      actividad->rutEmpresa = 0;

   if (Fget32(fml, CLI_DVX, 0, actividad->dv, 0) == -1)
      actividad->dv[0] = '\0';

   Fget32(fml, CLI_NOMBRE_EMPRESA, 0, nombreEmpresa, 0); nombreEmpresa[50] = '\0';
   strcpy(actividad->nombreEmpresa, nombreEmpresa);

   if (Fget32(fml, CLI_GIRO_EMPRESA, 0, (char *)&actividad->giroEmpresa, 0) == -1)
      actividad->giroEmpresa = 0;

   if (Fget32(fml, CLI_DIA_DE_PAGO, 0, (char *)&actividad->diaDePago, 0) == -1)
      actividad->diaDePago = 0;

   if (Fget32(fml, CLI_CARGO_ACTUAL, 0, (char *)&actividad->cargoActual, 0) == -1)
      actividad->cargoActual = 0;

   if (Fget32(fml, CLI_ANO_INGRESO, 0, (char *)&actividad->anoIngreso, 0) == -1)
      actividad->anoIngreso = 0;

   if (Fget32(fml, CLI_MES_INGRESO, 0, (char *)&actividad->mesIngreso, 0) == -1)
      actividad->mesIngreso = 0;

   if (Fget32(fml,CLI_EMPLEADOR_ANTERIOR,0,actividad->empleadorAnterior,0) == -1)
      actividad->empleadorAnterior[0] = '\0';

   if (Fget32(fml, CLI_CARGO_EMPLEADOR_ANTERIOR, 0,
                  (char *)&actividad->cargoEmpleadorAnterior, 0) == -1)
      actividad->cargoEmpleadorAnterior = 0;

   if (Fget32(fml, CLI_ANTIGUEDAD_ANOS_EMP_ANT, 0,
             (char *)&actividad->antiguedadAnosEmpAnt, 0) == -1)
      actividad->antiguedadAnosEmpAnt = 0;

   if (Fget32(fml, CLI_ANTIGUEDAD_MESES_EMP_ANT, 0,
             (char *)&actividad->antiguedadMesesEmpAnt , 0) == -1 )
      actividad->antiguedadMesesEmpAnt = 0;

   if (Fget32(fml, CLI_ES_EMPRESA_PRIME, 0, actividad->esEmpresaPrime, 0) == -1)
      actividad->esEmpresaPrime[0] = '\0';

/** Fin Modificacion 03-09-2014 (MARP) **/

   if (Foccur32(fml, CLI_ES_FUNCIONARIO_PUBLICO) > 0)
      Fget32(fml, CLI_ES_FUNCIONARIO_PUBLICO, 0, (char *)&actividad->esFuncionarioPublico, 0);

   if (actividad->rutEmpresa < 0) actividad->rutEmpresa = 0;
   if (strlen(actividad->dv) == 0) strcpy(actividad->dv," ");
   if (strlen(actividad->nombreEmpresa) == 0) strcpy(actividad->nombreEmpresa," ");
   if (actividad->giroEmpresa < 0) actividad->giroEmpresa = 0;
   if (actividad->diaDePago < 0) actividad->diaDePago = 0;
   if (actividad->cargoActual < 0) actividad->cargoActual = 0;
   if (actividad->anoIngreso < 0) actividad->anoIngreso = 0;
   if (actividad->mesIngreso < 0) actividad->mesIngreso = 0;
   if (strlen(actividad->empleadorAnterior) == 0) strcpy(actividad->empleadorAnterior," ");
   if (actividad->cargoEmpleadorAnterior < 0) actividad->cargoEmpleadorAnterior = 0;
   if (actividad->antiguedadAnosEmpAnt < 0) actividad->antiguedadAnosEmpAnt = 0;
   if (actividad->antiguedadMesesEmpAnt < 0) actividad->antiguedadMesesEmpAnt = 0;
   if (strlen(actividad->esEmpresaPrime) == 0) strcpy(actividad->esEmpresaPrime," ");
   if ((actividad->esFuncionarioPublico != 0) && (actividad->esFuncionarioPublico != 1)) actividad->esFuncionarioPublico = -1;

/** Inicio Modificacion 03-09-2014 (MARP) **/
   if (Fget32(fml, EVA_FML_MES_INI_EMPLEO_ANTERIOR, 0, (char *)&actividad->mesIniEmpAnterior, 0) == -1)    /** LEPF  **/
      actividad->mesIniEmpAnterior = 0;

   if (Fget32(fml, EVA_FML_ANO_INI_EMPLEO_ANTERIOR, 0, (char *)&actividad->anioIniEmpAnterior, 0) == -1)   /** LEPF  **/
      actividad->anioIniEmpAnterior = 0;

   if (Fget32(fml, EVA_FML_MES_FIN_EMPLEO_ANTERIOR, 0, (char *)&actividad->mesFinEmpAnterior, 0) == -1)    /** LEPF  **/
      actividad->mesFinEmpAnterior = 0;

   if (Fget32(fml, EVA_FML_ANO_FIN_EMPLEO_ANTERIOR, 0, (char *)&actividad->anioFinEmpAnterior, 0) == -1)   /** LEPF  **/
      actividad->anioFinEmpAnterior = 0;

/** Fin Modificacion 03-09-2014 (MARP) **/
}
 
 
void ObtieneRegistroActividad(FBFR32 *fml, tCliACTIVIDAD *actividad)
{
   Fadd32(fml, CLI_TIPO_EMPLEO, (char *)&actividad->tipoEmpleo, 0);
   Fadd32(fml, CLI_TIPO_RENTA, (char *)&actividad->tipoRenta, 0);
   Fadd32(fml, CLI_ACTIVIDAD, (char *)&actividad->actividad, 0);
   Fadd32(fml, CLI_RUT_EMPRESA, (char *)&actividad->rutEmpresa, 0);
   Fadd32(fml, CLI_DVX, actividad->dv, 0);
   Fadd32(fml, CLI_NOMBRE_EMPRESA, actividad->nombreEmpresa, 0);
   Fadd32(fml, CLI_GIRO_EMPRESA, (char *)&actividad->giroEmpresa, 0);
   Fadd32(fml, CLI_DIA_DE_PAGO, (char *)&actividad->diaDePago, 0);
   Fadd32(fml, CLI_CARGO_ACTUAL, (char *)&actividad->cargoActual, 0);
   Fadd32(fml, CLI_ANO_INGRESO, (char *)&actividad->anoIngreso, 0);
   Fadd32(fml, CLI_MES_INGRESO, (char *)&actividad->mesIngreso, 0);
   Fadd32(fml, CLI_EMPLEADOR_ANTERIOR, actividad->empleadorAnterior, 0);
   Fadd32(fml, CLI_CARGO_EMPLEADOR_ANTERIOR,
          (char *)&actividad->cargoEmpleadorAnterior, 0);
   Fadd32(fml, CLI_ANTIGUEDAD_ANOS_EMP_ANT,
          (char *)&actividad->antiguedadAnosEmpAnt, 0);
   Fadd32(fml, CLI_ANTIGUEDAD_MESES_EMP_ANT,
          (char *)&actividad->antiguedadMesesEmpAnt, 0);
   Fadd32(fml, CLI_ES_EMPRESA_PRIME, actividad->esEmpresaPrime, 0);
   Fadd32(fml, CLI_ES_FUNCIONARIO_PUBLICO, (char *)&actividad->esFuncionarioPublico, 0);
   Fadd32(fml, EVA_FML_MES_INI_EMPLEO_ANTERIOR, (char *)&actividad->mesIniEmpAnterior, 0);   /** LEPF **/
   Fadd32(fml, EVA_FML_ANO_INI_EMPLEO_ANTERIOR, (char *)&actividad->anioIniEmpAnterior, 0);  /** LEPF **/
   Fadd32(fml, EVA_FML_MES_FIN_EMPLEO_ANTERIOR, (char *)&actividad->mesFinEmpAnterior, 0);   /** LEPF **/
   Fadd32(fml, EVA_FML_ANO_FIN_EMPLEO_ANTERIOR, (char *)&actividad->anioFinEmpAnterior, 0);  /** LEPF **/
}
 
void LlenarRegistroDireccion(FBFR32 *fml, tCliDIRECCION *direccion)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&direccion->rutCliente, 0);
   Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccion->tipoDireccion, 0);
   Fget32(fml, CLI_CALLE_NUMERO, 0, direccion->calleNumero, 0);

   Fget32(fml, CLI_DEPARTAMENTO, 0, direccion->departamento, 0);
   if (strlen(direccion->departamento)==0) strcpy(direccion->departamento, " ");

   Fget32(fml, CLI_VILLA_POBLACION, 0, direccion->villaPoblacion, 0);
   if (strlen(direccion->villaPoblacion)==0) strcpy(direccion->villaPoblacion, " ");

   Fget32(fml, CLI_CIUDAD, 0, (char *)&direccion->ciudad, 0);
   Fget32(fml, CLI_COMUNA, 0, (char *)&direccion->comuna, 0);
   Fget32(fml, CLI_ZONA, 0, (char *)&direccion->zona, 0);
   Fget32(fml, CLI_CODIGO_POSTAL, 0, direccion->codigoPostal, 0);
   Fget32(fml, CLI_REFERENCIA, 0, direccion->referencia, 0);
   Fget32(fml, CLI_MOTIVO_MODIFICACION, 0, (char *)&direccion->motivoModificacion, 0);
}

void ObtenerRegistroDirecciones(FBFR32 *fml, tCliDIRECCION *direccion, 
                                int totalRegistros)
{
   int i;
   tCliDIRECCION dir;

   for (i=0; i<totalRegistros; i++)
 
   {
     dir = *(direccion + i);
     Fadd32(fml, CLI_TIPO_DIRECCION,(char *)&dir.tipoDireccion,0);
     Fadd32(fml, CLI_CALLE_NUMERO, dir.calleNumero,0);
     Fadd32(fml, CLI_DEPARTAMENTO, dir.departamento,0);
     Fadd32(fml, CLI_VILLA_POBLACION,dir.villaPoblacion,0);
     Fadd32(fml, CLI_CIUDAD,(char *)& dir.ciudad,0);
     Fadd32(fml, CLI_COMUNA,(char *)& dir.comuna,0);
     Fadd32(fml, CLI_ZONA,(char *)& dir.zona,0);
     Fadd32(fml, CLI_CODIGO_POSTAL, dir.codigoPostal,0);
     Fadd32(fml, CLI_TABLA_DESCRIPCION, dir.descripcionDireccion,0);
	 Fadd32(fml, CLI_REFERENCIA, dir.referencia,0);
	 Fadd32(fml, CLI_MOTIVO_MODIFICACION,(char *)&dir.motivoModificacion,0);
   }
}

 
void LlenarRegistroTelefono(FBFR32 *fml, tCliTELEFONO *telefono, int posicion)
{
   Fget32(fml, CLI_RUTX, posicion, (char *)&telefono->rutCliente, 0);
   Fget32(fml, CLI_TIPO_TELEFONO, posicion, (char *)&telefono->tipoTelefono, 0);
   Fget32(fml, CLI_NUMERO, posicion, telefono->numero, 0);

   /* ACD */
   if (Foccur32(fml, CLI_CODIGO_DE_AREA) > 0)
{ 
    userlog("PAP Tiene FML Cod Area");
    userlog("PAP %d", telefono->codigoArea);
       Fget32(fml, CLI_CODIGO_DE_AREA, posicion, (char *)&telefono->codigoArea  , 0);
}
   else
 {
    userlog("PAP NO Tiene FML Cod Area");
       telefono->codigoArea = 0;
 }
}
 
void ObtenerRegistroTelefonos(FBFR32 *fml, tCliTELEFONO *telefono, 
                              int totalRegistros)
{
   int i;
   tCliTELEFONO dir;
 
   for (i=0; i<totalRegistros; i++)
 
   {
     dir = *(telefono + i);
     Fadd32(fml, CLI_TIPO_TELEFONO,(char *)&dir.tipoTelefono,0);
     Fadd32(fml, CLI_NUMERO, dir.numero,0);
     Fadd32(fml, CLI_TABLA_DESCRIPCION, dir.descripcion,0);
     Fadd32(fml, CLI_CODIGO_DE_AREA,(char *)&dir.codigoArea,0);
   }
}

void LlenarRegistroRenta(FBFR32 *fml, tCliRENTA *renta, int posicion)
{
   Fget32(fml, CLI_RTA_TIPO, posicion, (char *)&renta->tipoMontoRenta, 0);
   Fget32(fml, CLI_RTA_MONTO, posicion, (char *)&renta->monto, 0);
}

void ObtenerRegistroRentas(FBFR32 *fml, tCliRENTA *renta,
                              int totalRegistros)
{
   int i;
   tCliRENTA rta;

   for (i=0; i<totalRegistros; i++)

   {
     rta = *(renta + i);
     Fadd32(fml, CLI_RTA_TIPO,(char *)&rta.tipoMontoRenta,0);
     Fadd32(fml, CLI_RTA_MONTO, (char *)&rta.monto,0);
     Fadd32(fml, CLI_TABLA_DESCRIPCION, rta.descripcion,0);
   }
}

void LlenarRegistroCuenta(FBFR32 *fml, tCliCUENTA *cuenta, int flag)
{
   long sts;

   /*flag = 1 para crear; flag = 2 para modificar*/
   /*los datos de info no se deben modificar*/
   if (flag == 2)
   {
     Fget32(fml, CLI_CORRELATIVO, 0, (char *)&cuenta->correlativo, 0);
   }
   Fget32(fml, CLI_RUTX, 0, (char *)&cuenta->rutCliente, 0);
   Fget32(fml, CLI_BANCO, 0, (char *)&cuenta->banco, 0);
   Fget32(fml, CLI_CUENTA, 0, cuenta->cuenta, 0);
}
 
void ObtenerRegistroCuentas(FBFR32 *fml, tCliCUENTA *cuenta, int totalRegistros)
{
   int i;
   tCliCUENTA dir;
 
   for (i=0; i<totalRegistros; i++)
 
   {
     dir = *(cuenta + i);
     Fadd32(fml, CLI_CORRELATIVO,(char *)&dir.correlativo,0);
     Fadd32(fml, CLI_BANCO, (char *)&dir.banco,0);
     Fadd32(fml, CLI_CUENTA, dir.cuenta,0);
   }
}
void  LlenarRegistroRegMod(FBFR32 *fml, tCliREGMOD *regModificacion, 
                           char *cadena)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&regModificacion->rutCliente, 0);

   if (Foccur32(fml, CLI_FECHA) > 0)
      Fget32(fml, CLI_FECHA, 0, regModificacion->fecha, 0);
   else
      strcpy(regModificacion->fecha, "01011900");   /* Esto no debiera ocurrir, es solo precaucion  - 03-09-2014 (MARP) */

   if (Foccur32(fml, CLI_USUARIO) > 0)
       Fget32(fml, CLI_USUARIO, 0, (char *)&regModificacion->usuario, 0);
   else
       regModificacion->usuario = 1;

   if (Foccur32(fml, CLI_SUCURSAL) > 0)
       Fget32(fml, CLI_SUCURSAL, 0, (char *)&regModificacion->sucursal, 0);
   else
       regModificacion->sucursal = 1;

   strcpy(regModificacion->modificacion, cadena);
}

void ObtenerRegistroModificacion(FBFR32 *fml, tCliREGMOD *regModificacion, int totalRegistros)
{
   int i;
   tCliREGMOD dir;
 
   for (i=0; i<totalRegistros; i++)
 
   {
     dir = *(regModificacion + i);
     Fadd32(fml, CLI_CORRELATIVO,(char *)&dir.correlativo,0);
     Fadd32(fml, CLI_USUARIO, (char *)&dir.usuario,0);
     Fadd32(fml, CLI_SUCURSAL, (char *)&dir.sucursal,0);
     Fadd32(fml, CLI_TIPO_MODIFICACION,(char *)&dir.tipoModificacion,0);
     Fadd32(fml, CLI_FECHA, dir.fecha,0);
     Fadd32(fml, CLI_MODIFICACION, dir.modificacion,0);
   }
}
 
void itoa(long numero, char palabra[20])
{
 char buff[20];
 int largo;

 sprintf(buff,"%li",numero);
 largo = strlen(buff);
 buff[largo+1]='\0';
 strncpy(palabra,buff,largo+1);
}
 
void ConcatenaPersonal(tCliCLIENTE cliente, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",cliente.rut);
  strcat(cadena, datoAux);
  
  cadena[0]='\0';
  sprintf(datoAux,"%1s",cliente.dv);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%30s",cliente.nombres);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%20s",cliente.apellidoPaterno);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%20s",cliente.apellidoMaterno);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%9s",cliente.fechaNacimiento);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.sexo);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",cliente.nacionalidad);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.estadoCivil);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.regimenMatrimonial);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.numCargasFamiliares);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.nivelEducacional);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",cliente.profesion);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",cliente.estado);
  strcat(cadena, datoAux);

}

/* ACD */
void ConcatenaCorreoElectronico(tCliCLIENTE cliente, char cadena2[201])
{
  char datoAux[60];

  memset(datoAux,0, sizeof(datoAux));
  
  cadena2[0]='\0';
  sprintf(datoAux,"%9li",cliente.rut);
  strcat(cadena2, datoAux);

  /* Tipo Correo 1, particular, se deja en duro, se cambiara a futuro */
  sprintf(datoAux," 1 %40s",cliente.direccionElectronica);
  strcat(cadena2, datoAux);

}
/* ACD */

void ConcatenaConyuge(tCliCONYUGE conyuge, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",conyuge.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%9li",conyuge.rutConyuge);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%1s",conyuge.dv);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%30s",conyuge.nombres);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%20s",conyuge.apellidoPaterno);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%20s",conyuge.apellidoMaterno);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",conyuge.profesion);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%1s",conyuge.trabaja);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%50s",conyuge.nombreEmpresa);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%12f",conyuge.rentaLiquida);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",conyuge.cargo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%2i",conyuge.mesIngreso);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",conyuge.anoIngreso);
  strcat(cadena, datoAux);
  
}

void ConcatenaActividad(tCliACTIVIDAD actividad, char cadena[221])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",actividad.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",actividad.tipoEmpleo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",actividad.actividad);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",actividad.tipoRenta);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",actividad.anoIngreso);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",actividad.mesIngreso);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%9li",actividad.rutEmpresa);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%1s",actividad.dv);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%50s",actividad.nombreEmpresa);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%1s",actividad.esEmpresaPrime);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",actividad.giroEmpresa);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",actividad.cargoActual);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%2i",actividad.diaDePago);
  strcat(cadena, datoAux);
  
}

void ConcatenaDireccion(tCliDIRECCION direccion, char cadena[201])
{
  char datoAux[64];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",direccion.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",direccion.tipoDireccion);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%40s",direccion.calleNumero);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%8s",direccion.departamento);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%40s",direccion.villaPoblacion);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",direccion.ciudad);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%4i",direccion.comuna);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%10s",direccion.codigoPostal);
  strcat(cadena, datoAux);
  
}

void ConcatenaTelefono(tCliTELEFONO telefono, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",telefono.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",telefono.tipoTelefono);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%20s",telefono.numero);
  strcat(cadena, datoAux);
  
  /* ACD */
  sprintf(datoAux,"%02d",telefono.codigoArea);
  strcat(cadena, datoAux);
  
}

void ConcatenaRenta(tCliRENTA renta, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",renta.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",renta.tipoMontoRenta);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",renta.monto);
  strcat(cadena, datoAux);

}


void ConcatenaCuenta(tCliCUENTA cuenta, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';
  sprintf(datoAux,"%9li",cuenta.rutCliente);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%9li",cuenta.correlativo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",cuenta.banco);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%20s",cuenta.cuenta);
  strcat(cadena, datoAux);
  
}

void ConcatenaBienValor(tCliBIENVALOR bienValor, char *cadena)
{
  char datoAux[60];

  sprintf(datoAux,"%9li",bienValor.rut);
  strcpy(cadena,datoAux);

  sprintf(datoAux,"%4i",bienValor.tipoBien);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%15.4f",bienValor.valorComercial);
  strcat(cadena,datoAux);
}

void ConcatenaBienRaiz(tCliBIENRAIZ bienRaiz, char *cadena)
{
  char datoAux[60];

  sprintf(datoAux,"%9li",bienRaiz.rutCliente);
  strcpy(cadena,datoAux);

  sprintf(datoAux,"%9li",bienRaiz.correlativo);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%60s",bienRaiz.ubicacionInmueble);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%30s",bienRaiz.tituloDominio);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%15.4f",bienRaiz.montoAvaluoFiscal);
  strcat(cadena,datoAux);
}

void ConcatenaAuto(tCliAUTO vehiculo, char *cadena)
{
  char datoAux[60];

  sprintf(datoAux,"%9li",vehiculo.rutCliente);
  strcpy(cadena,datoAux);

  sprintf(datoAux,"%9li",vehiculo.correlativo);
  strcat(cadena,datoAux);
  
  sprintf(datoAux,"%25s",vehiculo.marca);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%25s",vehiculo.modelo);
  strcat(cadena,datoAux);
  
  sprintf(datoAux,"%25s",vehiculo.patente);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%4i",vehiculo.ano);
  strcat(cadena,datoAux);

  sprintf(datoAux,"%15.4f",vehiculo.montoAvaluoFiscal);
  strcat(cadena,datoAux);
}

void ConcatenaVivienda(typeVivienda vivienda, char *cadena)
{
  char datoAux[60];
 
  sprintf(datoAux,"%4i",vivienda.tipoVivienda);
  strcpy(cadena,datoAux);
 
  sprintf(datoAux,"%4i",vivienda.antiguedadAnosEnVivienda);
  strcat(cadena,datoAux);
 
  sprintf(datoAux,"%4i",vivienda.antiguedadMesesEnVivienda);
  strcat(cadena,datoAux);
}

/********************Funciones para servicios IndRequerida*************/

void LlenarRegistroIndCliente(FBFR32 *fml, tCliINDCLIENTE *indCliente)
{
   Fget32(fml, CLI_SISTEMA, 0, indCliente->sistema, 0);

   Fget32(fml, CLI_FLAG_NOMBRES, 0, (char *)&indCliente->nombres, 0);
   Fget32(fml, CLI_FLAG_APELLIDO_PATERNO, 0, 
             (char *)&indCliente->apellidoPaterno, 0);
   Fget32(fml, CLI_FLAG_APELLIDO_MATERNO, 0, 
             (char *)&indCliente->apellidoMaterno, 0);
   Fget32(fml, CLI_FLAG_FECHA_NACIMIENTO, 0, 
             (char *)&indCliente->fechaNacimiento, 0);
   Fget32(fml, CLI_FLAG_SEXO, 0, 
             (char *)&indCliente->sexo, 0);
   Fget32(fml, CLI_FLAG_NACIONALIDAD, 0, 
             (char *)&indCliente->nacionalidad, 0);
   Fget32(fml, CLI_FLAG_ESTADO_CIVIL, 0, 
             (char *)&indCliente->estadoCivil, 0);
   Fget32(fml,CLI_FLAG_REGIMEN_MATRIMONIAL,0,
             (char *)&indCliente->regimenMatrimonial,0);
   Fget32(fml,CLI_FLAG_FECHA_MATRIMONIO,0,
             (char *)&indCliente->fechaMatrimonio,0);
   Fget32(fml, CLI_FLAG_NUM_CARGAS_FAMI, 0,
             (char *)&indCliente->numCargasFamiliares, 0);
   Fget32(fml, CLI_FLAG_NIVEL_EDUCACIONAL, 0,
             (char *)&indCliente->nivelEducacional, 0);
   Fget32(fml, CLI_FLAG_PROFESION, 0, (char *)&indCliente->profesion, 0);
 
}

void ObtieneRegistroIndCliente(FBFR32 *fml, tCliINDCLIENTE *indCliente)
{
   Fadd32(fml, CLI_FLAG_NOMBRES, (char *)&indCliente->nombres, 0);
   Fadd32(fml, CLI_FLAG_APELLIDO_PATERNO, 
             (char *)&indCliente->apellidoPaterno, 0);
   Fadd32(fml, CLI_FLAG_APELLIDO_MATERNO, 
             (char *)&indCliente->apellidoMaterno, 0);
   Fadd32(fml, CLI_FLAG_FECHA_NACIMIENTO, 
             (char *)&indCliente->fechaNacimiento, 0);
   Fadd32(fml, CLI_FLAG_SEXO, (char *)&indCliente->sexo, 0);
   Fadd32(fml, CLI_FLAG_NACIONALIDAD, (char *)&indCliente->nacionalidad, 0);
   Fadd32(fml, CLI_FLAG_ESTADO_CIVIL, (char *)&indCliente->estadoCivil, 0);
   Fadd32(fml, CLI_FLAG_REGIMEN_MATRIMONIAL,
             (char *)&indCliente->regimenMatrimonial, 0);
   Fadd32(fml, CLI_FLAG_FECHA_MATRIMONIO, 
             (char *)&indCliente->fechaMatrimonio, 0);
   Fadd32(fml, CLI_FLAG_NUM_CARGAS_FAMI,
             (char *)&indCliente->numCargasFamiliares, 0);
   Fadd32(fml, CLI_FLAG_NIVEL_EDUCACIONAL,
             (char *)&indCliente->nivelEducacional, 0);
   Fadd32(fml, CLI_FLAG_PROFESION, (char *)&indCliente->profesion, 0);
   Fadd32(fml, CLI_FLAG_CODIGOPAIS, (char *)&indCliente->codigopais, 0);
}

void LlenarRegistroIndConyuge(FBFR32 *fml, tCliINDCONYUGE *indConyuge)
{
   Fget32(fml, CLI_SISTEMA, 0, indConyuge->sistema, 0);

   Fget32(fml, CLI_FLAG_RUT_CONYUGE, 0, (char *)&indConyuge->rutConyuge, 0);
   Fget32(fml, CLI_FLAG_NOMBRES, 0, (char *)&indConyuge->nombres, 0);
   Fget32(fml, CLI_FLAG_APELLIDO_PATERNO, 0, 
             (char *)&indConyuge->apellidoPaterno, 0);
   Fget32(fml, CLI_FLAG_APELLIDO_MATERNO, 0, 
             (char *)&indConyuge->apellidoMaterno, 0);
   Fget32(fml, CLI_FLAG_PROFESION, 0, (char *)&indConyuge->profesion, 0);
   Fget32(fml, CLI_FLAG_TRABAJA, 0, (char *)&indConyuge->trabaja, 0);
   Fget32(fml, CLI_FLAG_NOMBRE_EMPRESA, 0, 
             (char *)&indConyuge->nombreEmpresa, 0);
   Fget32(fml, CLI_FLAG_RENTA_LIQUIDA, 0, 
             (char *)&indConyuge->rentaLiquida, 0);
   Fget32(fml, CLI_FLAG_ANO_INGRESO, 0, 
             (char *)&indConyuge->anoIngreso, 0);
   Fget32(fml, CLI_FLAG_MES_INGRESO, 0, 
             (char *)&indConyuge->mesIngreso, 0);
   Fget32(fml, CLI_FLAG_CARGO_ACTUAL, 0, 
             (char *)&indConyuge->cargo, 0);
}

void ObtieneRegistroIndConyuge(FBFR32 *fml, tCliINDCONYUGE *indConyuge)
{
   Fadd32(fml, CLI_FLAG_RUT_CONYUGE, (char *)&indConyuge->rutConyuge, 0);
   Fadd32(fml, CLI_FLAG_NOMBRES, (char *)&indConyuge->nombres, 0);
   Fadd32(fml, CLI_FLAG_APELLIDO_PATERNO, 
             (char *)&indConyuge->apellidoPaterno, 0);
   Fadd32(fml, CLI_FLAG_APELLIDO_MATERNO, 
             (char *)&indConyuge->apellidoMaterno, 0);
   Fadd32(fml, CLI_FLAG_PROFESION, (char *)&indConyuge->profesion, 0);
   Fadd32(fml, CLI_FLAG_TRABAJA, (char *)&indConyuge->trabaja, 0);
   Fadd32(fml, CLI_FLAG_NOMBRE_EMPRESA, (char *)&indConyuge->nombreEmpresa, 0);
   Fadd32(fml, CLI_FLAG_RENTA_LIQUIDA, (char *)&indConyuge->rentaLiquida, 0);
   Fadd32(fml, CLI_FLAG_ANO_INGRESO, (char *)&indConyuge->anoIngreso, 0);
   Fadd32(fml, CLI_FLAG_MES_INGRESO, (char *)&indConyuge->mesIngreso, 0);
   Fadd32(fml, CLI_FLAG_CARGO_ACTUAL, (char *)&indConyuge->cargo, 0);
}
 
void LlenarRegistroIndDireccion(FBFR32 *fml, tCliINDDIRECCION *indDireccion)
{
   Fget32(fml, CLI_SISTEMA, 0, indDireccion->sistema, 0);

   Fget32(fml, CLI_FLAG_TIPO_DIR, 0, (char *)&indDireccion->tipoDireccion, 0);
   Fget32(fml, CLI_FLAG_CALLE_NUMERO, 0, (char *)&indDireccion->calleNumero, 0);
   Fget32(fml, CLI_FLAG_DEPARTAMENTO, 0, (char *)&indDireccion->departamento, 0);
   Fget32(fml, CLI_FLAG_VILLA_POBLACION, 0, 
             (char *)&indDireccion->villaPoblacion, 0);
   Fget32(fml, CLI_FLAG_CIUDAD, 0, (char *)&indDireccion->ciudad, 0);
   Fget32(fml, CLI_FLAG_COMUNA, 0, (char *)&indDireccion->comuna, 0);
   Fget32(fml, CLI_FLAG_ZONA, 0, (char *)&indDireccion->zona, 0);
   Fget32(fml, CLI_FLAG_CODIGO_POSTAL, 0, 
             (char *)&indDireccion->codigoPostal, 0);
}
 
void ObtenerRegistroIndDirecciones(FBFR32 *fml, tCliINDDIRECCION *indDireccion)
{
   Fadd32(fml, CLI_FLAG_TIPO_DIR, 
             (char *)&indDireccion->tipoDireccion,0);
   Fadd32(fml, CLI_FLAG_CALLE_NUMERO, 
             (char *)&indDireccion->calleNumero,0);
   Fadd32(fml, CLI_FLAG_DEPARTAMENTO, 
             (char *)&indDireccion->departamento,0);
   Fadd32(fml, CLI_FLAG_VILLA_POBLACION,
             (char *)&indDireccion->villaPoblacion,0);
   Fadd32(fml, CLI_FLAG_CIUDAD, 
             (char *)&indDireccion->ciudad,0);
   Fadd32(fml, CLI_FLAG_COMUNA, 
             (char *)&indDireccion->comuna,0);
   Fadd32(fml, CLI_FLAG_ZONA, 
             (char *)&indDireccion->zona,0);
   Fadd32(fml, CLI_FLAG_CODIGO_POSTAL, 
             (char *)&indDireccion->codigoPostal,0);
}

void LlenarRegistroIndActividad(FBFR32 *fml, tCliINDACTIVIDAD *indActividad)
{
   Fget32(fml, CLI_SISTEMA, 0, indActividad->sistema, 0);
   Fget32(fml, CLI_FLAG_TIPO_EMPLEO, 0, 
             (char *)&indActividad->tipoEmpleo, 0);
   Fget32(fml, CLI_FLAG_TIPO_RENTA, 0, 
             (char *)&indActividad->tipoRenta, 0);
   Fget32(fml, CLI_FLAG_RUT_EMPRESA, 0, 
             (char *)&indActividad->rutEmpresa, 0);
   Fget32(fml, CLI_FLAG_NOMBRE_EMPRESA, 0, 
             (char *)&indActividad->nombreEmpresa, 0);
   Fget32(fml, CLI_FLAG_GIRO_EMPRESA, 0, 
             (char *)&indActividad->giroEmpresa, 0);
   Fget32(fml, CLI_FLAG_ES_EMPRESA_PRIME, 0, 
             (char *)&indActividad->esEmpresaPrime, 0);
   Fget32(fml, CLI_FLAG_DIA_DE_PAGO, 0, 
             (char *)&indActividad->diaDePago, 0);
   Fget32(fml, CLI_FLAG_CARGO_ACTUAL, 0, 
             (char *)&indActividad->cargoActual, 0);
   Fget32(fml, CLI_FLAG_ANO_INGRESO, 0, 
             (char *)&indActividad->anoIngreso, 0);
   Fget32(fml, CLI_FLAG_MES_INGRESO, 0, 
             (char *)&indActividad->mesIngreso, 0);
   Fget32(fml, CLI_FLAG_ACTIVIDAD, 0,
             (char *)&indActividad->actividad, 0);

}

void ObtieneRegistroIndActividad(FBFR32 *fml, tCliINDACTIVIDAD *indActividad)
{
   Fadd32(fml, CLI_FLAG_TIPO_EMPLEO, 
             (char *)&indActividad->tipoEmpleo, 0);
   Fadd32(fml, CLI_FLAG_TIPO_RENTA, 
             (char *)&indActividad->tipoRenta, 0);
   Fadd32(fml, CLI_FLAG_RUT_EMPRESA, 
             (char *)&indActividad->rutEmpresa, 0);
   Fadd32(fml, CLI_FLAG_NOMBRE_EMPRESA, 
             (char *)&indActividad->nombreEmpresa, 0);
   Fadd32(fml, CLI_FLAG_ES_EMPRESA_PRIME, 
             (char *)&indActividad->esEmpresaPrime, 0);
   Fadd32(fml, CLI_FLAG_GIRO_EMPRESA, 
             (char *)&indActividad->giroEmpresa, 0);
   Fadd32(fml, CLI_FLAG_DIA_DE_PAGO, 
             (char *)&indActividad->diaDePago, 0);
   Fadd32(fml, CLI_FLAG_CARGO_ACTUAL, 
             (char *)&indActividad->cargoActual, 0);
   Fadd32(fml, CLI_FLAG_ANO_INGRESO, 
             (char *)&indActividad->anoIngreso, 0);
   Fadd32(fml, CLI_FLAG_MES_INGRESO, 
             (char *)&indActividad->mesIngreso, 0);
   Fadd32(fml, CLI_FLAG_ACTIVIDAD,
             (char *)&indActividad->actividad, 0);
}

void LlenarRegistroIndTelefono(FBFR32 *fml, tCliINDTELEFONO *indTelefono)
{
   Fget32(fml, CLI_SISTEMA, 0, indTelefono->sistema, 0);
   Fget32(fml, CLI_FLAG_TIPO_TELEFONO, 0, 
             (char *)&indTelefono->tipoTelefono, 0);
   Fget32(fml, CLI_FLAG_NUMERO, 0, 
             (char *)&indTelefono->numero, 0);   
}
 
void ObtieneRegistroIndTelefono(FBFR32 *fml, tCliINDTELEFONO *indTelefono)
{
   Fadd32(fml, CLI_FLAG_TIPO_TELEFONO,
             (char *)&indTelefono->tipoTelefono,0);
   Fadd32(fml, CLI_FLAG_NUMERO, 
             (char *)&indTelefono->numero,0);
}

void LlenarRegistroIndRenta(FBFR32 *fml, tCliINDRENTA *indRenta)
{
   Fget32(fml, CLI_SISTEMA, 0, indRenta->sistema, 0);
   Fget32(fml, CLI_FLAG_RTA_TIPO, 0,
             (char *)&indRenta->tipoMontoRenta, 0);
   Fget32(fml, CLI_FLAG_RTA_MONTO, 0,
             (char *)&indRenta->monto, 0);
}

void ObtieneRegistroIndRenta(FBFR32 *fml, tCliINDRENTA *indRenta)
{
   Fadd32(fml, CLI_FLAG_RTA_TIPO,
             (char *)&indRenta->tipoMontoRenta,0);
   Fadd32(fml, CLI_FLAG_RTA_MONTO,
             (char *)&indRenta->monto,0);
}

void LlenarRegistroIndVivienda(FBFR32 *fml, tCliINDVIVIENDA *indVivienda)
{
   Fget32(fml, CLI_SISTEMA, 0, indVivienda->sistema, 0);
   Fget32(fml, CLI_FLAG_TIPO_VIVIENDA, 0, (char *)&indVivienda->tipoVivienda, 0);
   Fget32(fml, CLI_FLAG_ANTIGUEDAD_EN_ANOS_VIV, 0, (char *)&indVivienda->antiguedadEnAnos, 0);
   Fget32(fml, CLI_FLAG_ANTIGUEDAD_EN_MESES_VIV, 0,  (char *)&indVivienda->antiguedadEnMeses, 0);
}

void ObtieneRegistroIndVivienda(FBFR32 *fml, tCliINDVIVIENDA *indVivienda)
{
   Fadd32(fml, CLI_SISTEMA, indVivienda->sistema, 0);
   Fadd32(fml, CLI_FLAG_TIPO_VIVIENDA, (char *)&indVivienda->tipoVivienda, 0);
   Fadd32(fml, CLI_FLAG_ANTIGUEDAD_EN_ANOS_VIV, (char *)&indVivienda->antiguedadEnAnos, 0);
   Fadd32(fml, CLI_FLAG_ANTIGUEDAD_EN_MESES_VIV, (char *)&indVivienda->antiguedadEnMeses, 0);
}



void FMLSetearDesdeParametro(FBFR32 *fml, tCliPARAMETRO *parametro)
{
   Fadd32(fml, CLI_FECHA_PROC_LINEA, parametro->fechaProcLinea, 0);
   Fadd32(fml, CLI_FECHA_PROC_BATCH, parametro->fechaProcBatch, 0);
   Fadd32(fml, CLI_VERSION, parametro->ultimaVersion, 0);
   Fadd32(fml, CLI_VERSION_DLL, parametro->ultimaVersionDLL, 0);
}



void LlenarRegistroBienValor(FBFR32 *fml, tCliBIENVALOR *bienValor, int ptr)
{
   Fget32(fml,CLI_BIEN_TIPO,ptr,(char *)&bienValor->tipoBien,0);
   Fget32(fml,CLI_BIEN_VALOR_COMERCIAL,ptr,(char *)&bienValor->valorComercial,0);
}


void FMLSetearBienesValores(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliBIENVALOR *bienValor;
  int i;
 
  for (i=0; i<(lista->count); i++)
  {
     bienValor = (tCliBIENVALOR *)QGetItem(lista,i);
 
     Fadd32(fml,CLI_BIEN_TIPO,(char *)&bienValor->tipoBien, 0);
     Fadd32(fml,CLI_BIEN_VALOR_COMERCIAL,(char *)&bienValor->valorComercial, 0);
  }
}

void LlenarRegistroBienRaiz(FBFR32 *fml, tCliBIENRAIZ *bienRaiz, int ptr, int mod)
{
   if (mod)
      Fget32(fml,CLI_CORRELATIVO,ptr,(char *)&bienRaiz->correlativo,0);

   Fget32(fml,CLI_CASA_UBICACION,ptr,bienRaiz->ubicacionInmueble,0);
   Fget32(fml,CLI_CASA_TITULO,ptr,bienRaiz->tituloDominio,0);
   Fget32(fml,CLI_CASA_AVALUO_FISCAL,ptr,(char *)&bienRaiz->montoAvaluoFiscal,0);
   Fget32(fml,CLI_BIEN_VALOR_COMERCIAL,ptr,
                        (char *)&bienRaiz->montoValorComercial,0);
}


void FMLSetearBienRaiz(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliBIENRAIZ *bienRaiz;
  int i;
 
  for (i=0; i<(lista->count); i++)
  {
     bienRaiz = (tCliBIENRAIZ *)QGetItem(lista,i);
 
     Fadd32(fml,CLI_CORRELATIVO,(char *)&bienRaiz->correlativo, 0);
     Fadd32(fml,CLI_CASA_UBICACION,bienRaiz->ubicacionInmueble, 0);
     Fadd32(fml,CLI_CASA_TITULO,bienRaiz->tituloDominio, 0);
     Fadd32(fml,CLI_CASA_AVALUO_FISCAL,(char *)&bienRaiz->montoAvaluoFiscal, 0);
     Fadd32(fml,CLI_BIEN_VALOR_COMERCIAL,
        (char *)&bienRaiz->montoValorComercial, 0);
  }
}


void LlenarRegistroAuto(FBFR32 *fml, tCliAUTO *vehiculo, int ptr, int mod)
{
   if (mod)
      Fget32(fml,CLI_CORRELATIVO,ptr,(char *)&vehiculo->correlativo,0);

   Fget32(fml,CLI_AUTO_MARCA,ptr,vehiculo->marca,0);
   Fget32(fml,CLI_AUTO_MODELO,ptr,vehiculo->modelo,0);
   Fget32(fml,CLI_AUTO_PATENTE,ptr,vehiculo->patente,0);
   Fget32(fml,CLI_AUTO_ANO,ptr,(char *)&vehiculo->ano,0);
   Fget32(fml,CLI_AUTO_AVALUO_FISCAL,ptr,(char *)&vehiculo->montoAvaluoFiscal,0);
   Fget32(fml,CLI_BIEN_VALOR_COMERCIAL,ptr,
               (char *)&vehiculo->montoValorComercial,0);
}


void FMLSetearAuto(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliAUTO *vehiculo;
  int i;
 
  for (i=0; i<(lista->count); i++)
  {
     vehiculo = (tCliAUTO *)QGetItem(lista,i);
 
     Fadd32(fml,CLI_CORRELATIVO,(char *)&vehiculo->correlativo, 0);
     Fadd32(fml,CLI_AUTO_MARCA,vehiculo->marca, 0);
     Fadd32(fml,CLI_AUTO_MODELO,vehiculo->modelo, 0);
     Fadd32(fml,CLI_AUTO_PATENTE,vehiculo->patente, 0);
     Fadd32(fml,CLI_AUTO_ANO,(char *)&vehiculo->ano, 0);
     Fadd32(fml,CLI_AUTO_AVALUO_FISCAL,(char *)&vehiculo->montoAvaluoFiscal, 0);
     Fadd32(fml,CLI_BIEN_VALOR_COMERCIAL,
        (char *)&vehiculo->montoValorComercial, 0);
  }
}


void AsignarDeFMLAVivienda(FBFR32 *fml, typeVivienda *vivienda)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&vivienda->rutCliente, 0);
   Fget32(fml, CLI_TIPO_VIVIENDA, 0, (char *)&vivienda->tipoVivienda, 0);
   Fget32(fml, CLI_ANT_ANOS_EN_VIVIENDA, 0, 
             (char *)&vivienda->antiguedadAnosEnVivienda, 0);
   Fget32(fml, CLI_ANT_MESES_EN_VIVIENDA, 0, 
             (char *)&vivienda->antiguedadMesesEnVivienda, 0);

}


void AsignarAFMLDeVivienda(FBFR32 *fml, typeVivienda *vivienda)
{
   Fadd32(fml, CLI_TIPO_VIVIENDA, (char *)&vivienda->tipoVivienda, 0);
   Fadd32(fml, CLI_ANT_ANOS_EN_VIVIENDA, 
             (char *)&vivienda->antiguedadAnosEnVivienda, 0);
   Fadd32(fml, CLI_ANT_MESES_EN_VIVIENDA, 
             (char *)&vivienda->antiguedadMesesEnVivienda, 0);

}

void AsignarDeFMLASuper(FBFR32 *fml, typeSuperIntendencia *super)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&super->rutCliente, 0);
   Fget32(fml, CLI_SI_CATEGORIA_DEUDOR, 0, (char *)&super->categoriaDeudor, 0);
   super->composicionInstitucional = CLIENTE_NATURAL;
   super->actividadEconomica = 0;
   strcpy(super->razon, CLI_UN_BLANCO);
   strcpy(super->clasificacionDeudor, DEUDOR_NUEVO );
}


void AsignarAFMLDeSuper(FBFR32 *fml, typeSuperIntendencia *super)
{
   Fadd32(fml, CLI_SI_CATEGORIA_DEUDOR, (char *)&super->categoriaDeudor, 0);
}


void CalcularDiasDeDiferencia(char *iFechaVencimiento, char *iFechaPago, short *oDiasDiferencia)
{
   short diferenciaDias=0;

   DiferenciaFechas(iFechaVencimiento,iFechaPago, &diferenciaDias);
   *oDiasDiferencia = diferenciaDias;
}

int CalcularMoraPromedio (Q_HANDLE  *iDiferenciaFechas)
{
   int  moraPromedio=0, i;
   int  *diferenciaFecha;

   if (iDiferenciaFechas->count <= 0)
     return (moraPromedio);
   else
   {
     for (i=0; i<iDiferenciaFechas->count; i++)
     {
        diferenciaFecha = (int *)QGetItem(iDiferenciaFechas,i);
        if (*diferenciaFecha > 0)
          moraPromedio += *diferenciaFecha;
     }
     moraPromedio = moraPromedio/iDiferenciaFechas->count;
   }
   return (moraPromedio);
}

long TUXRecuperarFechaProcesoYCalendario(char *oFechaProceso, char *oFechaCalendario, char *oMensajeError)
{
   FBFR32    *fml;
   char    mensaje[500];
   long    largo=0, sts=0;


   *oFechaProceso = '\0';  *oFechaCalendario = '\0';
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

long TUXClienteOperaciones (long rutCliente,  Q_HANDLE *listaOperaciones)
{
   FBFR32    *fml;
   char    mensaje[500];
   long    largo=0, sts=0;
   short   colocaciones=1,i=0, totalRegistros=0;
   tCliOPERACIONES *operacion;

   listaOperaciones->itemLen = sizeof(tCliOPERACIONES);

   if (( fml = (FBFR32 *)tpalloc("FML32",NULL,1024)) == NULL)
   {
     userlog("CLIENTE ERROR -  RUT: (%li), MENSAJE ERROR: (%s)\n", rutCliente, "Falló al asignar memoria");
     return -1;
   }

   Fadd32(fml, CLI_CLIENTE_RUT, (char *)&rutCliente, 0);
   Fadd32(fml, CLI_OPERACION_CONSIDERAR_VIGENCIA, "S", 0);
   Fadd32(fml, CLI_OPERACION_CLASIFICACION, (char *)&colocaciones, 0);

   if (( sts = tpcall("CliOperaciones", (char *)fml, 0L, (char **)&fml, &largo, TPNOTIME)) == -1)
   {
      Fget32( fml, HRM_MENSAJE_SERVICIO,  0, mensaje, 0);
      userlog("CLIENTE ERROR -  RUT: (%li), MENSAJE ERROR: (%s)\n", rutCliente, mensaje);
      tpfree((char *)fml);
      return -1;
   }

   totalRegistros = Foccur32(fml, CLI_OPERACION_CODIGO);
   for (i=0; i < totalRegistros ; i++)
   {
      if ((operacion = (tCliOPERACIONES *)QMakeItem(listaOperaciones)) == NULL){
         return -1;
      }

      Fget32(fml, CLI_OPERACION_CODIGO,i, (char *)&operacion->credito, 0);
      Fget32(fml, CLI_OPERACION_NUMERO,i, operacion->numeroCredito, 0);
   }
   tpfree((char *)fml);
   return(0);
}

long TUXClienteCuotasOperacionCRC (double numeroOperacion,  Q_HANDLE *listaOperaciones)
{
   FBFR32    *fml;
   char    mensaje[500];
   long    largo=0, sts=0;
   short   colocaciones=1,i=0, totalRegistros=0;
   tCliCREDITOCUOTA *operacion;

   listaOperaciones->itemLen = sizeof(tCliCREDITOCUOTA);

   if (( fml = (FBFR32 *)tpalloc("FML32",NULL,1024)) == NULL)
   {
     userlog("CLIENTE ERROR -  OPERACION: (%f), MENSAJE ERROR: (%s)\n", numeroOperacion, "Falló al asignar memoria");
     return -1;
   }

   Fadd32(fml, CRC_CODIGO_CREDITO, (char *)&numeroOperacion, 0);

   if (( sts = tpcall("CrcRecCuotas", (char *)fml, 0L, (char **)&fml, &largo, TPNOTIME)) == -1)
   {
      Fget32( fml, HRM_MENSAJE_SERVICIO,  0, mensaje, 0);
      userlog("CLIENTE ERROR -  OPERACION: (%f), MENSAJE ERROR: (%s)\n", numeroOperacion, mensaje);
      tpfree((char *)fml);
      return -1;
   }

   totalRegistros = Foccur32(fml, CRC_NUMERO_CUOTA);
   for (i=0; i < totalRegistros ; i++)
   {
      if ((operacion = (tCliCREDITOCUOTA *)QMakeItem(listaOperaciones)) == NULL){
         return -1;
      }
      operacion->credito = numeroOperacion;
      Fget32(fml, CRC_NUMERO_CUOTA,i, (char *)&operacion->cuota, 0);
      Fget32(fml, CRC_FECHA_VENCIMIENTO,i, operacion->fechaVencimiento, 0);
      Fget32(fml, CRC_FECHA_PAGO,i, operacion->fechaPago, 0);
      CalcularDiasDeDiferencia(operacion->fechaVencimiento, operacion->fechaPago, &operacion->moraDias);
   }
   tpfree((char *)fml);
   return(0);
}

/****f* cliente/servidores/fuentes
  *  NAME
  *    DeterminarComposicionInstitucional.
  *  AUTHOR
  *    Consuelo Montenegro.
  *  CREATION DATE
  *    20/06/2001.
  *  DESCRIPTION
  *	   Dado el rut se identifica a que tipoEstado pertenece.
  *  PARAMETERS
  *		rut.
  *  RETURN VALUE
  *    Estratificacion de Composicion Institucional (Ver cliente/servidores/include/cliente.h).
  *  MODIFICATION HISTORY
  *		Para un rut especifico, se asigna un tipoEstado especifico.
  *		Guillermo Parra A.
  *		08-10-2007.
  ******
  *
  *
  */

long DeterminarComposicionInstitucional(long lngRut, short *ptrOTipoEstrato)
{    
	*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_6;
	
	if (lngRut == CLI_BANCO_FALABELLA)
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_4;
		return(0);
	}
	
	if (((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_AD1) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_AH1)) ||
		((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_AD2) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_AH2)) ||
		((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_AD3) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_AH3)))
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_1;
		return(0);
	}
	
	if ((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_AD4) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_AH4))
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_2;
		return(0);
	}
	
	if ((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_AD5) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_AH5))
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_3;
		return(0);
	}
	
	if ((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_BD6) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_BH6))
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_4;
		return(0);
	}
	
	if ((lngRut >= CLI_COMP_INSTITUCIONAL_GRUPO_BD7) && (lngRut <= CLI_COMP_INSTITUCIONAL_GRUPO_BH7))
	{
		*ptrOTipoEstrato = CLI_TIPO_ESTRATO_COMP_INSTITUCIONAL_5;
		return(0);
	}
	
	return(0);
}

/**************************************************************************/
/* Objetivo : Valida  la clave de acceso a Internet de un usuario         */
/* Autor    : Boris Contreras Mac-lean                  Fecha: 08-05-2002 */
/**************************************************************************/
long ValidarPinInternet(long rut, char *pinIngresado, short sucursal, char *mensajeError)
{
   FBFR32 *fml;
   long largo;

   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml, CLI_RUT,          (char *)&rut, 0);
   Fadd32(fml, CLI_PIN_INTERNET, pinIngresado, 0);
   Fadd32(fml, CLI_SUCURSAL,     (char *)&sucursal, 0);

   if (tpcall("CliAutValPinInt", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
       Fget32(fml, HRM_MENSAJE_SERVICIO, 0, mensajeError, 0);
       tpfree((char *)fml);
       return(tpurcode);
   }

   tpfree((char *)fml);
   return(tpurcode);
}

/**************************************************************************/
/* Objetivo : Recupera el Pin Offset de una clave Internet                */
/* Autor    : Boris Contreras mac-lean                  Fecha: 08-05-2002 */
/**************************************************************************/
long RecuperarPinOffsetInternet(long rut, char *pinIngresado, char *pinOffset, char *mensajeError)
{
   FBFR32 *fml;
   long largo;
   char offsetChar[CLI_LARGO_BLOCK_INTERNET + 1];

   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml, CLI_RUT         , (char *)&rut, 0);
   Fadd32(fml, CLI_PIN_INTERNET, pinIngresado, 0);

   if (tpcall("CliCalPinOffInt", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
       Fget32(fml, HRM_MENSAJE_SERVICIO, 0, mensajeError, 0);
       tpfree((char *)fml);
       return(tpurcode);
   }

   Fget32(fml, CLI_PIN_OFFSET, 0, offsetChar, 0);

   strcpy(pinOffset, offsetChar);

   tpfree((char *)fml);
   return(tpurcode);
}


/*******************************************************************/
/* Objetivo : Valida la clave de acceso Ivr de un usuario          */
/* Autor    : Boris Contreras Mac-lean           Fecha: 08-05-2002 */
/*******************************************************************/
long ValidarPinIvr(long rut, char *pinIngresado, short sucursal, char *mensajeError)
{
   FBFR32 *fml;
   long largo;
   short offset;

   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml, CLI_RUT,      (char *)&rut, 0);
   Fadd32(fml, CLI_PIN_IVR,  pinIngresado, 0);
   Fadd32(fml, CLI_SUCURSAL, (char *)&sucursal, 0);

   if (tpcall("CliAutValPinIvr", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
       Fget32(fml, HRM_MENSAJE_SERVICIO, 0, mensajeError, 0);
       tpfree((char *)fml);
       return(tpurcode);
   }

   tpfree((char *)fml);
   return(tpurcode);
}

/*********************************************************************/
/* Objetivo : Recupera el Pin Offset de una clave Ivr                */
/* Autor    : Boris Contreras mac-lean             Fecha: 08-05-2002 */
/*********************************************************************/
long RecuperarPinOffsetIvr(long rut, char *pinIngresado, long *pinOffset, char *mensajeError)
{
   FBFR32 *fml;
   long largo;
   char offsetChar[CLI_LARGO_PIN_IVR + 1];

   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml, CLI_RUT,       (char *)&rut, 0);
   Fadd32(fml, CLI_PIN_IVR,   pinIngresado, 0);

   if (tpcall("CliCalPinOffIvr", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
       Fget32(fml, HRM_MENSAJE_SERVICIO, 0, mensajeError, 0);
       tpfree((char *)fml);
       return(tpurcode);
   }

   Fget32(fml, CLI_PIN_OFFSET, 0, offsetChar, 0);

   *pinOffset = atol(offsetChar);

   tpfree((char *)fml);
   return(tpurcode);
}


/**************************************************************************/
/* Objetivo : Recuperar solicitud cambio de dirección                     */
/* Autor    : Jan Riega Z.                              Fecha: 07-11-2002 */
/**************************************************************************/

long RecuperarNuevaDireccion(tTcrSOLICITUDCLIENTE *solicitudCliente)
{
   FBFR32 *fml;
   long largo;

   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml, TCR_CORRELATIVO , (char *)&solicitudCliente->correlativo, 0);


   if (tpcall("TcrRecSolCamDir", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
      userlog("RecuperarNuevaDireccion, SOLICITUD - CORRELATIVO: (%li)\n",solicitudCliente->correlativo);
      tpfree((char *)fml);
      return(SRV_CRITICAL_ERROR);
   }

   Fget32(fml, TCR_RUTCLIENTE, 0, (char *)&solicitudCliente->rutCliente, 0);
   Fget32(fml, TCR_DIRECCIONDEENVIO, 0, (char *)&solicitudCliente->nuevaDireccion, 0);
   Fget32(fml, TCR_OBSERVACION, 0, solicitudCliente->observacion, 0);
   Fget32(fml, TCR_TARJETACREDITO, 0, solicitudCliente->numeroTarjeta, 0);

   tpfree((char *)fml);
   return(SRV_SUCCESS);
}

long obtenerInicioDireccion(char *direcciones,short *j)
{
short  k;

   k= *j;
   while((direcciones[k] != '@')  && (direcciones[k] != '\0')) k++;

   if ( (direcciones[k+1] == '&') || (direcciones[k] = '\0')) return(SRV_NOT_FOUND);

   *j = k+1;

   return(SRV_SUCCESS);

}

long determinarTipoDireccion(short i,tCliDIRECCION *direccion,tCliTELEFONO *telefono)
{
    switch(i)
   {
      case 0:  {
                   direccion->tipoDireccion =  CLI_DIRECCION_PARTICULAR;
                   telefono->tipoTelefono = CLI_TELEFONO_PARTICULAR;
                   strcpy(direccion->descripcionDireccion,"Dirección particular");
                   break;
               }

      case 1:  {
                   direccion->tipoDireccion =  CLI_DIRECCION_COMERCIAL;
                   telefono->tipoTelefono   =  CLI_TELEFONO_COMERCIAL;
                   strcpy(direccion->descripcionDireccion,"Dirección comercial");
                    break;
               }

      case 2:  {
                   direccion->tipoDireccion =  CLI_DIRECCION_OTRA;
                   telefono->tipoTelefono   =  CLI_TELEFONO_OTRO;
                   strcpy(direccion->descripcionDireccion,"Otra dirección");
                   break;
               }
    }

   return(SRV_SUCCESS);
}



long  DesconcatenaDireccion(tTcrSOLICITUDCLIENTE solicitudCliente,short i,short *inicioDireccionAnterior,tCliDIRECCION *direccion,
                                                                                                         tCliTELEFONO *telefono)
{
char   direcciones[650+1];
short  j;
short  campo;
short  k;
short  inicioCampo;
short  inicioComuna;
short  inicioCiudad;
long   sts;
int    pp=0;
char   codigoCiudad[4+1];
char   codigoComuna[4+1];

   j = *inicioDireccionAnterior;
   strcpy(direcciones,solicitudCliente.observacion);
   direcciones[strlen(direcciones)] = '\0';

  /* Este ciclo permite obtener el inicio de una nueva dirección, las cuales estan separadas por el caracter '@' */

   sts = obtenerInicioDireccion(direcciones,&j);

   if ( sts != SRV_SUCCESS) return(SRV_NOT_FOUND);

   *inicioDireccionAnterior=j;

   campo=0;
   /* direccion->rutCliente = solicitudCliente.rutCliente; */
   direccion->zona = 0;
   strcpy(direccion->villaPoblacion," ");
   strcpy(direccion->departamento," ");

   determinarTipoDireccion(i,direccion,telefono);

   while((direcciones[j] != '@') && (direcciones[j] != '\0') && (campo <5) )
   {

           inicioCampo=j;
           while((direcciones[j] != '&')  && (direcciones[j] != '@')  && (direcciones[j] != '\0')) j++;
           if ((direcciones[j] == '\0')  && (campo>4)){ printf("\n salio con not_found");return(SRV_NOT_FOUND);}


           switch(campo)
           {
                 case 0: {
                     strncpy(direccion->calleNumero,direcciones+inicioCampo,j-inicioCampo);
                     direccion->calleNumero[j-inicioCampo] = '\0';
                     printf("\n el j donde termina el campo en case 0 es: %d",j);
                     campo++;
                     printf("\n direccion->calleNumero: %s",direccion->calleNumero);
                     break;
                 }

                 case 1: {
                     k=inicioCampo;
                     while((direcciones[k] != '-') && (direcciones[k] != '\0') ) k++;
                     if (direcciones[k] == '\0') return(SRV_NOT_FOUND);
                     k++;
                     strncpy(codigoCiudad,direcciones+inicioCampo,k-inicioCampo);
                     codigoCiudad[k-inicioCampo] = '\0';
                     direccion->ciudad = atoi(codigoCiudad);
                     if (direccion->ciudad == 0) return(SRV_NOT_FOUND);

                     printf("\n el valor de inicioCampo (inicio campo case 1) : %d",inicioCampo);
                     printf("\n el valor de k (fin  campo case 1) : %d",k);

                     printf("\n direccion->ciudad: %d",direccion->ciudad);
                     campo++;
                     break;
                 }

                 case 2: {
                     k=inicioCampo;
                     while ((direcciones[k] != '-') && (direcciones[k] != '\0' )) k++;
                     if (direcciones[k] == '\0') return(SRV_NOT_FOUND);
                     k++;

                     strncpy(codigoComuna,direcciones+inicioCampo,k-inicioCampo);
                     codigoCiudad[k-inicioCampo] = '\0';
                     direccion->comuna = atoi(codigoComuna);
                     if (direccion->comuna == 0) return(SRV_NOT_FOUND);

                     printf("\n el valor de inicioCampo (inicio campo case 2) : %d",inicioCampo);
                     printf("\n el valor de k (fin  campo case 2) : %d",k);

                     printf("\n direccion->comuna: %d",direccion->comuna);
                     campo++;
                     break;
                  }

                case 3: {
                    strncpy(direccion->codigoPostal,direcciones+inicioCampo,j-inicioCampo);
                    direccion->codigoPostal[j-inicioCampo] = '\0';
                    printf("\n el valor de j (fin campo case 3) : %d",j);
                    printf("\n el valor de inicioCampo (inicio  campo case 3) : %d",inicioCampo);

                    if (strcmp(direccion->codigoPostal,"") == 0) strcpy(direccion->codigoPostal," ");
                    printf("\n direccion->codigoPostal: %s",direccion->codigoPostal);
                    campo++;
                    break;
                  }

                case 4: {
                    strncpy(telefono->numero,direcciones+inicioCampo,j-inicioCampo);
                    telefono->numero[j-inicioCampo] = '\0';

                    printf("\n el valor de j (fin campo case 4) : %d",k);
                    printf("\n el valor de inicioCampo (inicio  campo case 4) : %d",inicioCampo);

                    printf("\n telefono->numero: %s",telefono->numero);
                    campo++;
                    break;
                  }
              }
          j++;
  }
   return(SRV_SUCCESS);
}

void  LlenarRegistroModificacion(tTcrSOLICITUDCLIENTE solicitudCliente,char *fechaCalendario,char *cadena, tCliREGMOD *regModificacion)
{
   regModificacion->rutCliente = solicitudCliente.rutCliente;
   regModificacion->usuario = CLI_USUARIO_INTERNET;
   regModificacion->sucursal = CLI_SUCURSAL_INTERNET;
   strcpy(regModificacion->modificacion, cadena);
   strncpy(regModificacion->fecha,fechaCalendario,8);
   regModificacion->fecha[8] = '\0';
}


long  ModificarModoProcesoSolicitudCliente(long correlativo)
{
   FBFR32          *fml;
   char            mensajeError[500];
   long            largo=0, sts=0;


   fml = (FBFR32 *)tpalloc("FML32",NULL, 1024);

   Fadd32(fml,TCR_CORRELATIVO , (char *)&correlativo, 0);

   if (tpcall("TcrModModProSol", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
       Fget32(fml, HRM_MENSAJE_SERVICIO, 0, mensajeError, 0);
       tpfree((char *)fml);
       return(tpurcode);
   }

   tpfree((char *)fml);
   return(tpurcode);
}

/* [Ini]:Veto General de rut */
long RecuperarClienteVetado(long rutCliente, char *respuesta)
{
   FBFR32    *fml;
   long    largo;

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
   strcpy(respuesta,"NO");
   Fadd32(fml,EVA_RUT,(char *)&rutCliente, 0);

   if (tpcall("SgtRecCliVet", (char *)fml, 0L, (char **)&fml, &largo, TPNOTRAN) == -1)
   {
      tpfree((char *)fml);
      if (tperrno == TPENOENT)
      {
         userlog("Falló al recuperar Servicio SgtRecCliVet del servidor EvaParam");
      }
   }
   else
   {
      strcpy(respuesta,"SI");
   }
   /* userlog("* respuesta : %s",respuesta); */

   tpfree((char *)fml);
   return SRV_SUCCESS;
}
/* [Fin]:Veto General de rut */

/****************  FUNCIONES NUEVAS EN REFERENCIA  ****************/

void LlenarRegistroClienteReferencia(FBFR32 *fml, tCliREFERENCIA *referencia)
{
   /* ACD cod area */
   Fget32(fml, CLI_RUTX, 0, (char *)&referencia->rutCliente, 0);
   Fget32(fml, CLI_RUT, 0, (char *)&referencia->rutPariente, 0);
   Fget32(fml, CLI_DIGVER, 0, referencia->dv, 0);
   Fget32(fml, CLI_CODIGO_PARENTESCO, 0, (char *)&referencia->codigoParentesco, 0);
   Fget32(fml, CLI_NOMBRES, 0, referencia->nombres, 0);
   Fget32(fml, CLI_APELLIDO_PATERNO, 0, referencia->apellidoPaterno, 0);
   Fget32(fml, CLI_APELLIDO_MATERNO, 0, referencia->apellidoMaterno, 0);
   Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&referencia->tipoDireccion, 0);
   Fget32(fml, CLI_CALLE_NUMERO, 0, referencia->calleNumero, 0);
   Fget32(fml, CLI_DEPARTAMENTO, 0, referencia->departamento, 0);
   Fget32(fml, CLI_VILLA_POBLACION, 0, referencia->villaPoblacion, 0);
   Fget32(fml, CLI_CIUDAD, 0, (char *)&referencia->ciudad, 0);
   Fget32(fml, CLI_COMUNA, 0, (char *)&referencia->comuna, 0);
   Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&referencia->tipoTelefono, 0);
   Fget32(fml, CLI_NUMERO, 0, referencia->numero, 0);
   Fget32(fml, CLI_CODIGO_DE_AREA, 0, (char *)&referencia->codigoArea, 0);
   /* ACD */
   if (Foccur32(fml, CLI_CODIGO_DE_AREA) > 0)
       Fget32(fml, CLI_CODIGO_DE_AREA, 0, (char *)&referencia->codigoArea, 0);
   else
       referencia->codigoArea  = 0;
}

/****************  FUNCIONES NUEVAS EN REFERENCIA FIN  ****************/

void ConcatenaGastoPasivo(tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';

  sprintf(datoAux,"%9li",gastosPasivosBienesActivos.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%2i",gastosPasivosBienesActivos.codigo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%2i",gastosPasivosBienesActivos.tipo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",gastosPasivosBienesActivos.pagoMensual);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",gastosPasivosBienesActivos.pagoTotal);
  strcat(cadena, datoAux);

}


void LlenarRegistroDetalleUbicacionInmuebleES(FBFR32 *fml, tCliDETALLEUBICACIONINMUEBLE *detalleUbicacionInmueble)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&detalleUbicacionInmueble->rutCliente, 0);
   Fget32(fml, CLI_DETALLE_BIENES_EST_SIT, 0, detalleUbicacionInmueble->detalleBienes, 0);
   Fget32(fml, CLI_ROL_NUM_EST_SIT, 0, (char *)&detalleUbicacionInmueble->rolNum, 0);
   Fget32(fml, CLI_ROL_NUM2_EST_SIT, 0, (char *)&detalleUbicacionInmueble->rolNum2, 0);
}

void ConcatenaUbiInmueble(tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmueble, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';

  sprintf(datoAux,"%9li",detalleUbicacionInmueble.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%40s",detalleUbicacionInmueble.detalleBienes);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%9li",detalleUbicacionInmueble.rolNum);
  strcat(cadena, datoAux);
  
  sprintf(datoAux,"%9li",detalleUbicacionInmueble.rolNum2);
  strcat(cadena, datoAux);
}

void LlenarRegistroTituloDominioES(FBFR32 *fml, tCliTITULODOMINIO *tituloDominio)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&tituloDominio->rutCliente, 0);
   Fget32(fml, CLI_FOJAS_EST_SIT, 0, tituloDominio->fojas, 0);
   Fget32(fml, CLI_NUMERO_EST_SIT, 0, tituloDominio->numero, 0);
   Fget32(fml, CLI_ANO_EST_SIT, 0, (char *)&tituloDominio->ano, 0);
   Fget32(fml, CLI_AVALUO_FISCAL_EST_SIT, 0, (char *)&tituloDominio->avaluoFiscal, 0);
}

void ConcatenaTituloDominio(tCliTITULODOMINIO tituloDominio, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';

  sprintf(datoAux,"%9li",tituloDominio.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%30s",tituloDominio.fojas);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%30s",tituloDominio.numero);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",tituloDominio.ano);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",tituloDominio.avaluoFiscal);
  strcat(cadena, datoAux);
}

void LlenarRegistroVehiculoES(FBFR32 *fml, tCliVEHICULOESTSIT *vehiculoEstSit)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&vehiculoEstSit->rutCliente, 0);
   Fget32(fml, CLI_MARCA_EST_SIT, 0, vehiculoEstSit->marca, 0);
   Fget32(fml, CLI_MODELO_EST_SIT, 0, vehiculoEstSit->modelo, 0);
   Fget32(fml, CLI_ANO_EST_SIT, 0, (char *)&vehiculoEstSit->ano, 0);
   Fget32(fml, CLI_AVALUO_FISCAL_EST_SIT, 0, (char *)&vehiculoEstSit->avaluoFiscal, 0);
   Fget32(fml, CLI_PATENTE_NUM_EST_SIT, 0, vehiculoEstSit->patenteNum, 0);
}

void ConcatenaVehiculo(tCliVEHICULOESTSIT vehiculoEstSit, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';

  sprintf(datoAux,"%9li",vehiculoEstSit.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%25s",vehiculoEstSit.marca);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%25s",vehiculoEstSit.modelo);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%4i",vehiculoEstSit.ano);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",vehiculoEstSit.avaluoFiscal);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%6s",vehiculoEstSit.patenteNum);
  strcat(cadena, datoAux);
}

void LlenarRegistroBienAcreedorES(FBFR32 *fml, tCliBIENACTIVOACRE *bienAcreedor)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&bienAcreedor->rutCliente, 0);
   Fget32(fml, CLI_APELLIDOPATERNO, 0, bienAcreedor->apellidoPaterno, 0);
   Fget32(fml, CLI_APELLIDOMATERNO, 0, bienAcreedor->apellidoMaterno, 0);
   Fget32(fml, CLI_NOMBRE, 0, bienAcreedor->nombres, 0);
   Fget32(fml, CLI_RUT, 0, (char *)&bienAcreedor->rut, 0);
   Fget32(fml, CLI_DIGVER, 0, bienAcreedor->dv, 0);
   Fget32(fml, CLI_INTACREE_EST_SIT, 0, bienAcreedor->intAcreedor, 0);
   Fget32(fml, CLI_MONTODEUDA_EST_SIT, 0, (char *)&bienAcreedor->montodeuda, 0);
}

void ConcatenaBienAcreedor(tCliBIENACTIVOACRE bienAcreedor, char cadena[201])
{
  char datoAux[60];

  cadena[0]='\0';

  sprintf(datoAux,"%9li",bienAcreedor.rutCliente);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%20s",bienAcreedor.apellidoPaterno);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%20s",bienAcreedor.apellidoMaterno);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%30s",bienAcreedor.nombres);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%9li",bienAcreedor.rut);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%1s",bienAcreedor.dv);
  strcat(cadena, datoAux);
 
  sprintf(datoAux,"%40s",bienAcreedor.intAcreedor);
  strcat(cadena, datoAux);

  sprintf(datoAux,"%15.4f",bienAcreedor.montodeuda);
  strcat(cadena, datoAux);
}

void LlenarRegistroBuscarVehiculoES(FBFR32 *fml, tCliVEHICULOESTSIT *vehiculoEstSit)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&vehiculoEstSit->rutCliente, 0);
   Fget32(fml, CLI_PATENTE_NUM_EST_SIT, 0, vehiculoEstSit->patenteNum, 0);
}

void LlenarRegistroBuscarTitDomES(FBFR32 *fml, tCliTITULODOMINIO *tituloDominio)
{
   Fget32(fml, CLI_RUTX, 0, (char *)&tituloDominio->rutCliente, 0);
   Fget32(fml, CLI_FOJAS_EST_SIT, 0, tituloDominio->fojas, 0);
}

void FMLSetearDetalleBienInmuebleCli(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliDETALLEUBICACIONINMUEBLE *detalleUbicacionInmueble;
  int i;

  for (i=0; i<(lista->count); i++)
  {
     detalleUbicacionInmueble = (tCliDETALLEUBICACIONINMUEBLE *)QGetItem(lista,i);

     Fadd32(fml,CLI_RUTX,(char *)&detalleUbicacionInmueble->rutCliente, 0);
     Fadd32(fml,CLI_DETALLE_BIENES_EST_SIT,detalleUbicacionInmueble->detalleBienes, 0);
     Fadd32(fml,CLI_ROL_NUM_EST_SIT,(char *)&detalleUbicacionInmueble->rolNum, 0);
     Fadd32(fml,CLI_ROL_NUM2_EST_SIT,(char *)&detalleUbicacionInmueble->rolNum2, 0);
  }
}

void FMLSetearTituloDominioCli(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliTITULODOMINIO *tituloDominio;
  int i;

  for (i=0; i<(lista->count); i++)
  {
     tituloDominio = (tCliTITULODOMINIO *)QGetItem(lista,i);

     Fadd32(fml,CLI_RUTX,(char *)&tituloDominio->rutCliente, 0);
     Fadd32(fml,CLI_FOJAS_EST_SIT,tituloDominio->fojas, 0);
     Fadd32(fml,CLI_NUMERO_EST_SIT,tituloDominio->numero, 0);
     Fadd32(fml,CLI_ANO_EST_SIT,(char *)&tituloDominio->ano, 0);
     Fadd32(fml,CLI_AVALUO_FISCAL_EST_SIT,(char *)&tituloDominio->avaluoFiscal, 0);
  }
}

void FMLSetearVehiculoCli(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliVEHICULOESTSIT *vehiculoEstSit;
  int i;

  for (i=0; i<(lista->count); i++)
  {
     vehiculoEstSit= (tCliVEHICULOESTSIT *)QGetItem(lista,i);

     Fadd32(fml,CLI_RUTX,(char *)&vehiculoEstSit->rutCliente, 0);
     Fadd32(fml,CLI_MARCA_EST_SIT,vehiculoEstSit->marca, 0);
     Fadd32(fml,CLI_MODELO_EST_SIT,vehiculoEstSit->modelo, 0);
     Fadd32(fml,CLI_ANO_EST_SIT,(char *)&vehiculoEstSit->ano, 0);
     Fadd32(fml,CLI_AVALUO_FISCAL_EST_SIT,(char *)&vehiculoEstSit->avaluoFiscal, 0);
     Fadd32(fml,CLI_PATENTE_NUM_EST_SIT,vehiculoEstSit->patenteNum, 0);
  }
}

void FMLSetearBienAcreedorCli(FBFR32 *fml, Q_HANDLE *lista)
{
  tCliBIENACTIVOACRE *bienAcreedor;
  int i;

  for (i=0; i<(lista->count); i++)
  {
     bienAcreedor = (tCliBIENACTIVOACRE *)QGetItem(lista,i);

     Fadd32(fml,CLI_RUTX,(char *)&bienAcreedor->rutCliente, 0);
     Fadd32(fml,CLI_APELLIDOPATERNO,bienAcreedor->apellidoPaterno, 0);
     Fadd32(fml,CLI_APELLIDOMATERNO,bienAcreedor->apellidoMaterno, 0);
     Fadd32(fml,CLI_NOMBRE,bienAcreedor->nombres, 0);
     Fadd32(fml,CLI_RUT,(char *)&bienAcreedor->rut, 0);
     Fadd32(fml,CLI_DIGVER, bienAcreedor->dv, 0);
     Fadd32(fml,CLI_INTACREE_EST_SIT,bienAcreedor->intAcreedor, 0);
     Fadd32(fml,CLI_MONTODEUDA_EST_SIT,(char *)&bienAcreedor->montodeuda, 0);
  }
}

/*************************************************************************************/
/*************** ACTUALIZACION INTENTOS FALLIDOS IVR *********************************/
/*************************************************************************************/

int PIN_CLIOK(tCliCLAVEDEACCESOIVR *CliClaveAcceso)
{
        int sts;

        sts = SqlCliReiniciarIntentosFallidosClaveIvr(CliClaveAcceso);
        if (sts != SQL_SUCCESS)
        {
                if (sts == SQL_NOT_FOUND)
                {
                        printf("\nPIN_CCOK: CC.Validacion por Hardware Insertar accesos fallidos.\n");
                        sts = SqlCliInsertarClaveDeAccesoIvrF2(CliClaveAcceso);
                        if (sts != SQL_SUCCESS)
                        {
                                printf("\nPIN_CLIOK: CLI. Validacion por Hardware Error al insertar accesos fallidos.\n");
                                return (SRV_CRITICAL_ERROR);
                        }
                        else
                        {
                                sts = SRV_SUCCESS;
                        }
                }
                else
                {
                        printf("\nPIN_CLIOK: CLI. Error al eliminar accesos fallidos.\n");
                        return (SRV_CRITICAL_ERROR);
                }
        }
        else
        {
            sts = SRV_SUCCESS;
        }
        return(sts);
}

int PIN_CLINOOK(tCliCLAVEDEACCESOIVR *CliClaveAcceso)
{
        int sts;

        sts = SqlCliIncrementarIntentosFallidosClaveIvr(CliClaveAcceso);
        if (sts != SQL_SUCCESS)
        {
                printf("CLI. Error al incrementar los accesos fallidos.\n");
                return (SRV_CRITICAL_ERROR);
        }

        if (CliClaveAcceso->intentosFallidos < CANTIDAD_MAXIMA_ACCESO_FALLIDOS - 1)
        {
                printf("CLI.pin no valido.\n");
                return (SRV_FOUND);          /* realiza commit */
        }
        else if (CliClaveAcceso->intentosFallidos == CANTIDAD_MAXIMA_ACCESO_FALLIDOS - 1)
        {
                printf("CLI.Advertecia Bloqueo Clave Ivr.\n");
                return (SRV_FOUND);         /* realiza commit */
        }
        else if (CliClaveAcceso->intentosFallidos >= CANTIDAD_MAXIMA_ACCESO_FALLIDOS)
        {
                sts = SqlCliBloquearClaveDeAccesoIvr(CliClaveAcceso);
                if (sts != SQL_SUCCESS)
                {
                        printf("CLI.Error al bloquear clave de acceso.\n");
                        return (SRV_CRITICAL_ERROR);
                }
                printf("CLI.Clave Bloqueada.\n");
                return (SRV_FOUND);         /* realiza commit */
        }
        printf("Cli.No entra por las condiciones.\n");
        return (SRV_CRITICAL_ERROR);
        
}

/* ACD ini */

void ObtenerRegistroPrefijos(FBFR32 *fml, tCliPREFIJOSCELULAR *prefijos,
                              int totalRegistros)
{
   int i;
   tCliPREFIJOSCELULAR pre;

   for (i=0; i<totalRegistros; i++)

   {
     pre = *(prefijos + i);
     Fadd32(fml, CLI_TABLA_DESCRIPCION, pre.descripcion,0);
     Fadd32(fml, CLI_CODIGO_DE_AREA,(char *)&pre.codigo,0);
   }
}

void ObtenerRegistroExtension(FBFR32 *fml, tCliEXTCORREO *extensionCorreo, 
                              int totalRegistros)
{
   int i;
   tCliEXTCORREO dir;
 
   for (i=0; i<totalRegistros; i++)
 
   {
     dir = *(extensionCorreo + i);
     Fadd32(fml, CLI_EXT_CORREO_CODIGO,(char *)&dir.codigo,0);
     Fadd32(fml, CLI_EXT_CORREO_EXTENSION, dir.extension,0);
     Fadd32(fml, CLI_EXT_CORREO_DESCRIPCION, dir.descripcion,0);
   }
}

/* ACD fin */

/* Implantación de Normativa FATCA PJVA INI */
void AsignarDeFMLAClienteFATCA(FBFR32 *fml, tCliClienteFATCA *clienteFatca)
{
   
   Fget32(fml, CLI_RUTX, 0,                         (char *)&clienteFatca->rut, 0);
   Fget32(fml, CLI_OPERACION, 0,                    (char *)&clienteFatca->operacion, 0);
   Fget32(fml, CLI_FECHA_CREACION, 0,               clienteFatca->fechaCreacion, 0);
   Fget32(fml, CLI_CODIGO_CLASIFICACION_FATCA, 0,   (char *)&clienteFatca->codigoClasificacionFatca, 0);
   Fget32(fml, CLI_CODIGO_CLASIFICACION_DOC, 0,     (char *)&clienteFatca->codigoClasificacionDocumental, 0);
   Fget32(fml, CLI_FECHA_CLASIFICACION, 0,          clienteFatca->fechaClasificacionFatca, 0);
   Fget32(fml, CLI_FECHA_ESTADO_DOCUMENTAL, 0,      clienteFatca->fechaClasificacionDocumental, 0);
   Fget32(fml, CLI_USUARIO_EJECUTIVO, 0,            (char *)&clienteFatca->ejecutivo, 0);
   Fget32(fml, CLI_SUCURSAL, 0,                     (char *)&clienteFatca->sucursal, 0);
   Fget32(fml, CLI_CODIGO_SISTEMA, 0,               (char *)&clienteFatca->sistema, 0);
   Fget32(fml, CLI_TIN, 0,                          clienteFatca->tin, 0);
   Fget32(fml, CLI_COMENTARIO, 0,                   clienteFatca->comentario, 0);
   Fget32(fml, CLI_ES_VIGENTE, 0,                   clienteFatca->esVigente, 0);
   Fget32(fml, CLI_FECHA_EXPIRACION, 0,             clienteFatca->fechaExpiracion, 0);

}

void AsignarDeFMLADetalleClienteFATCA(FBFR32 *fml, tCliDetalleClienteFATCA *detalleClienteFatca)
{

   Fget32(fml, CLI_CORRELATIVO, 0,                  (char *)&detalleClienteFatca->correlativo, 0);
   Fget32(fml, CLI_OPERACION, 0,                    (char *)&detalleClienteFatca->operacion, 0);
   Fget32(fml, CLI_CODIGO_DOCUMENTO, 0,             (char *)&detalleClienteFatca->tipoDocumento, 0);
   Fget32(fml, CLI_RUTX, 0,                         (char *)&detalleClienteFatca->rut, 0);
   Fget32(fml, CLI_FECHA_CREACION, 0,               detalleClienteFatca->fechaCreacion, 0);
   Fget32(fml, CLI_FECHA_ENTREGA, 0,                detalleClienteFatca->fechaEntrega, 0);
   Fget32(fml, CLI_FECHA_TOPE, 0,                   detalleClienteFatca->fechaTope, 0);
   Fget32(fml, CLI_CODIGO_CLASIFICACION_DOC, 0,     (char *)&detalleClienteFatca->codigoEstadoDocumento, 0);
   Fget32(fml, CLI_USUARIO_EJECUTIVO, 0,            (char *)&detalleClienteFatca->ejecutivo, 0);
   Fget32(fml, CLI_SUCURSAL, 0,                     (char *)&detalleClienteFatca->sucursal, 0);
   Fget32(fml, CLI_ES_VIGENTE, 0,                   detalleClienteFatca->esVigente, 0);   
   
}
/* Implantación de Normativa FATCA PJVA FIN */
