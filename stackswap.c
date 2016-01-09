
/*
**
**  $VER: stackswap.c 1.1 (12.11.97)
**  anim.datatype 1.1
**
**  Dispatch routine for a DataTypes class
**
**  Written 1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/* main includes */
#include "classbase.h"

/*****************************************************************************/

typedef ULONG ASM (*DMHOOKFUNC)( REGA0 struct Hook *, REGA2 APTR, REGA1 APTR );

struct MyStackSwapStruct
{
    struct StackSwapStruct  stk;
    struct IClass          *cl;
    Object                 *o;
    Msg                     msg;
};

/*****************************************************************************/


DISPATCHERFLAGS
ULONG StackSwapDispatch( REGA0 struct IClass *cl, REGA2 Object *o, REGA1 Msg msg )
{
    struct ClassBase         *cb = (struct ClassBase *)(cl -> cl_UserData);
    ULONG                     retval;
    struct MyStackSwapStruct  mystk;
    UBYTE                    *lower,
                             *upper,
                             *sp;
    struct Task              *ThisTask;
    ULONG                     stacksize,
                              required_stacksize = (ULONG)(cl -> cl_Dispatcher . h_Data);

    /* Fill in data */
    mystk . cl                = cl;
    mystk . o                 = o;
    mystk . msg               = msg;

    ThisTask = FindTask( NULL );
    stacksize = (ULONG)(((UBYTE *)(ThisTask -> tc_SPReg)) - ((UBYTE *)(ThisTask -> tc_SPLower)));

    /* Enougth stack ? */
    if( stacksize > ((required_stacksize * 2UL) / 3UL) )
    {
      retval = MyDispatch( (&mystk) );
    }
    else
    {
      /* Alloc a new stack frame... */
      while( !(lower = (UBYTE *)AllocMem( required_stacksize, MEMF_PUBLIC )) );

      sp = upper = lower + required_stacksize;

      mystk . stk . stk_Lower   = lower;
      mystk . stk . stk_Upper   = (ULONG)upper;
      mystk . stk . stk_Pointer = sp;

      retval = SwapMe( (&mystk) );

      FreeMem( lower, required_stacksize );
    }

    return( retval );
}


/* swap stack */
DISPATCHERFLAGS
ULONG SwapMe( REGA0 struct MyStackSwapStruct *mystk )
{
    register ULONG retval;

#define cb ((struct ClassBase *)(mystk -> cl -> cl_UserData))

    StackSwap( (&(mystk -> stk)) );

      retval = MyDispatch( mystk );

    StackSwap( (&(mystk -> stk)) );

#undef cb

    return( retval );
}


/* call class dispatcher */
DISPATCHERFLAGS
ULONG MyDispatch( REGA0 struct MyStackSwapStruct *mystk )
{
    struct IClass *cl  = mystk -> cl;

    return( (*((DMHOOKFUNC)(cl -> cl_Dispatcher . h_SubEntry)))( (&(cl -> cl_Dispatcher)), (APTR)(mystk -> o), (APTR)(mystk -> msg) ) );
}


