/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middle-ware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "definitions.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
#define APP_MOUNT_NAME          "/mnt/myDrive1"
#define APP_DEVICE_NAME         "/dev/mtda1"
#define APP_FS_TYPE                     LITTLEFS
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
// *****************************************************************************

/* Work buffer used by FAT FS during Format */
uint8_t CACHE_ALIGN work[SYS_FS_LFS_MAX_SS];

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

/*This enum holds example application related NVM operation states*/

typedef enum 
{
    /* FTP over LFS service NVM write state */
    SYS_LFSwFTP_NVM_WRITE = 0,

    /* FTP over LFS service NVM erase state */
    SYS_LFSwFTP_NVM_ERASE,

    /* FTP over LFS service NVM read state */
    SYS_LFSwFTP_NVM_READ,
            
    /* FTP over LFS service NVM invalid state */
    SYS_LFSwFTP_NONE = 255
            
} SYS_LFSwFTP_NVMTYPEOPER; //NVM-type operation

// *****************************************************************************

typedef struct 
{
    /* FTP over LFS service NVM type operation  */
    SYS_LFSwFTP_NVMTYPEOPER nvmTypeOfOperation;
} SYS_LFSwFTP_OBJ; /*FTP over LFS system service Object*/

// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************

/*FTP over LFS Object */
static  SYS_LFSwFTP_OBJ      g_LFSwFTPObj = {SYS_LFSwFTP_NONE};

/* FTP over LFS service related 'LFS mount and format complete structure instance to read flag details from and write to*/
static  SYS_LFSwFTP_CONFIG   g_LFSwFTPflag CACHE_ALIGN;

/* FTP over LFS service related isLFSmountFormatSuccess flag read details*/
static  SYS_LFSwFTP_CONFIG   g_LFSwFTPflagRead;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/

static void SYS_LFSwFTP_NVMRead_Verify(void)
{
        /* When NVM read provide empty data, the returned data value will be 0xFF */
    
        if((g_LFSwFTPflagRead.isLFSmountFormatSuccess) && (g_LFSwFTPflagRead.FSused))
        {       
                /* NVM read provided valid data */
                memcpy(&g_LFSwFTPflag, &g_LFSwFTPflagRead, sizeof (SYS_LFSwFTP_CONFIG));
                memset(&g_LFSwFTPflagRead, 0, sizeof (SYS_LFSwFTP_CONFIG));
                appData.state = APP_STATE_SERVICE_TASKS;
        }
        else 
        {     
                /* NVM read provided invalid data. So, formatting the disk */
                appData.state = APP_STATE_FORMAT_DISK;
        }
}

static inline void SYS_LFSwFTP_NVMRead(void) 
{
        /* Start the NVM read operation */
        g_LFSwFTPObj.nvmTypeOfOperation = SYS_LFSwFTP_NVM_READ;  
        NVM_Read((uint32_t *)&g_LFSwFTPflagRead, sizeof (g_LFSwFTPflag), SYS_LFSwFTP_EXAMP_NVMADDR);
}

static inline void SYS_LFSwFTP_NVMWrite(void)
{
        /* Start the NVM Write operation */
        g_LFSwFTPObj.nvmTypeOfOperation = SYS_LFSwFTP_NVM_WRITE;
        NVM_SingleDoubleWordWrite((uint32_t *)&g_LFSwFTPflag, SYS_LFSwFTP_EXAMP_NVMADDR);
}

static inline void SYS_LFSwFTP_NVMErase(void) 
{
        /* Start the NVM Erase operation */
        g_LFSwFTPObj.nvmTypeOfOperation = SYS_LFSwFTP_NVM_ERASE;
        NVM_PageErase(SYS_LFSwFTP_EXAMP_NVMADDR);
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;



    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

SYS_FS_FORMAT_PARAM opt;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            if(TCPIP_STACK_Status(sysObj.tcpip) == SYS_STATUS_READY)
            {
                appData.state = APP_STATE_MOUNT_DISK;
            }
            bool appInitialized = false;
            if (appInitialized)
            {
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_MOUNT_DISK:
        {
            /* Mount the disk */
            if(SYS_FS_Mount(APP_DEVICE_NAME, APP_MOUNT_NAME, APP_FS_TYPE, 0, NULL) != 0)
            {
                /* The disk could not be mounted. Try mounting again until
                     * the operation succeeds. */
                SYS_CONSOLE_PRINT("[ERR] - Mount Failed : In APP_MOUNT_DISK State\r\n");
                appData.state = APP_STATE_MOUNT_DISK;
            }
            else
            {      
                /* Mount was successful. Format the disk. */
                SYS_CONSOLE_PRINT("Mount LFS Success!!\r\n");
                appData.state = APP_STATE_NVM_Read;
            }
            break;
        }
        
        case APP_STATE_NVM_Read:
        {
            opt.fmt = SYS_FS_FORMAT_FAT;
            opt.au_size = 0;
            /* Check if NVM flash is performing any operation */
            if(!NVM_IsBusy())
            {
                SYS_LFSwFTP_NVMRead();
                /* verify the NVM Read output */
                SYS_LFSwFTP_NVMRead_Verify();
            }
            break;
        }
        
        case APP_STATE_FORMAT_DISK:
        {
            if (SYS_FS_DriveFormat (APP_MOUNT_NAME, &opt, (void *)work, SYS_FS_LFS_MAX_SS) != SYS_FS_RES_SUCCESS)
            {
                /* Format of the disk failed. */
                SYS_CONSOLE_PRINT("[ERR] - Format Failed : In APP_FORMAT_DISK State\r\n");
                appData.state = APP_STATE_ERROR;
            }
            else
            {
                SYS_CONSOLE_PRINT("Formant LFS Success!!\r\n");                
                /* Format succeeded. Proceed to set "isLFSmountFormatSuccess" flag*/
                g_LFSwFTPflag.FSused = LittleFS;
                g_LFSwFTPflag.isLFSmountFormatSuccess = true;
                appData.state =  APP_STATE_NVM_Erase;
            }
            break;
        }
        
        case APP_STATE_NVM_Erase:
        {
            /* Check if NVM flash is performing any operation */
            if (!NVM_IsBusy()) 
            {
                /* Start the NVM Erase Operation */
                SYS_LFSwFTP_NVMErase();               
                appData.state = APP_STATE_NVM_Write;
            }   
            break;
        }
        
        case APP_STATE_NVM_Write:
        {
            /* Check if NVM flash performing any operation */
            if (!NVM_IsBusy()) 
            {
                /* Start the NVM Write operation */
                SYS_LFSwFTP_NVMWrite();                      
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }
        
        case APP_STATE_SERVICE_TASKS:
        {
            break;
        }

        /* TODO: implement your application state machine.*/
        case APP_STATE_ERROR:
        {
            /* The application comes here when the demo has failed.*/
            break;
        }

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
