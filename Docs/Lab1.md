# Lab1

Booting a PC

## Inline Assembly

This part is my notes for *Brennn's Guide to Inline Assembly* which is recommanded by the official tutorial.

This inline assembly is under DJGPP. DJGPP is based on GCC, so it uses the AT&T/UNIX syntax.

* Register names are prefixed with "%".

  ```assembly
  AT&T: %eax
  Intel: eax
  ```

* The source is always on the left, and the destination is always on the right.

  ```assembly
  AT&T: movl %eax, %ebx
  Intel: mov ebx, eax
  ```

* All constant/immediate values are prefixed with "$".

  ```assembly
  AT&T: movl $_booga, %eax
  Intel: mov eax, _booga
  ```

* You must suffix the instruction with one of **b**, **w**, or **l** to specify the width of the destination register as a **byte**, **word** or **longword**.

  The equivalent forms for Intel is **byte ptr**, **word ptr**, and **dword ptr**.

  ```assembly
  AT&T: movw %ax, %bx
  Intel: mov bx, ax
  ```

* Referencing memory: immed32 + basepointer + indexpointer * indexscale

  ```assembly
  AT&T: immed32(basepointer, indexpointer, indexscale)
  Intel: [basepointer + indexpointer * indexscale + immed32]
  ```

  * Addressing a particular C variable

    ```assembly
    AT&T: _booga
    Intel: [_booga]
    ```

    `_` is how to get at static (global) C variables from assembler. This only works with global variables.

  * Addressing what a register points to

    ```assembly
    AT&T: (%eax)
    Intel: [eax]
    ```

  * Addressing a variable offset by a value in a register

    ```assembly
    AT&T: _variable(%eax)
    Intel: [eax + _variable]
    ```

  * Addressing a value in an array of integers

    ```assembly
    AT&T: _array(,%eax,4)
    Intel: [eax*4 + array]
    ```

    

## PC Bootstrap

* PC's physical address space

```shell

+------------------+  <- 0xFFFFFFFF (4GB)
|      32-bit      |
|  memory mapped   |
|     devices      |
|                  |
/\/\/\/\/\/\/\/\/\/\

/\/\/\/\/\/\/\/\/\/\
|                  |
|      Unused      |
|                  |
+------------------+  <- depends on amount of RAM
|                  |
|                  |
| Extended Memory  |
|                  |
|                  |
+------------------+  <- 0x00100000 (1MB)
|     BIOS ROM     |
+------------------+  <- 0x000F0000 (960KB)
|  16-bit devices, |
|  expansion ROMs  |
+------------------+  <- 0x000C0000 (768KB)
|   VGA Display    |
+------------------+  <- 0x000A0000 (640KB)
|                  |
|    Low Memory    |
|                  |
+------------------+  <- 0x00000000

```

* RAM can somewhat equal to main memory.
* 0x00000000 - 0x000A0000 is RAM and 0x000A0000 - 0x000FFFFF is ROM early PCs used or updateable flash memory current PCs use.
* The most important region  from 0x000A0000 through 0x000FFFFF is BIOS (Basic Input/Output System), which occupies the 64KB region from 0x000F0000 through 0x000FFFFF.
* BIOS is responsible for performing basic system initialization such as activating the video card and checking the amount of memory installed. After performing this initialization, the BIOS loads the OS from some appropriate location and passes control of the machine to the OS.
* Nowadays PCs supporting more larger address spaces still follow the architects used by early PCs which were only capable of addressing 1MB of physical memory, to ensure backward compatibility with existing software.
* Modern PCs have a "hole" in physical memory from 0x000A0000 to 0x00100000, dividing RAM into "low" or "conventional memory" (the first 640KB) and "extended memory" (everything else).
* Some space at the very top of the PC's 32-bit physical address space, above all physical RAM, is now commonly reserved by the BIOS for use by 32-bit PCI devices.
* One thing isn't mentioned in the tutorial is **why 32-bit PCs need 4G address space**s. The reason is that one CPU calculation needs 32 bit for 32-bit PCs. The address space should satify every possible of commands stored in. So the address space is $2^{32} = 4294967296 = 4G$.
* For PCs with more than 4GB of physical RAM, BIOS must arrange to leave a second hole in the system's RAM at the top of the 32-bit addressable region, to leave room for these 32-bit devices to be mapped.
* 

