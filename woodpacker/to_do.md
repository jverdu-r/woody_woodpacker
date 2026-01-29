1-Requirements recap

    *Input: one ELF64 binary.
    *Output: new binary woody.
    *Encrypted payload must decrypt at runtime, print “....WOODY....” then run
        identically.
    *Generate random key and print it on packer run.
2-Project structure

    *Separate modules: ELF parsing, payload selection, encryption, stub injection, output
        writing, utils/error handling.
3-ELF parsing

    *Validate ELF magic, 64-bit class, x86_64.
    *Read Elf64_Ehdr, program headers, section headers if needed.
4-Choose encryption target

    *Identify an executable segment to encrypt (typically .text or executable PT_LOAD).
    *Decide exact byte range and store its offset/size.
5-Encryption algorithm

    *Pick a non-trivial algorithm (e.g., stream cipher with XOR + key schedule).
    *Generate random key; store it in the packed binary.
    *Print key in the packer output.
6-Stub design

    *A small runtime decryptor:
    *Prints “....WOODY....”
    *Decrypts the payload in memory
    *Jumps to original entry point
7-Injection strategy

    *Add a new executable segment or extend an existing one.
    *Place stub + key + metadata.
    *Update ELF headers: entry point, program headers, segment sizes, alignment.
8-Relocation/positioning

    *Ensure stub uses RIP-relative addressing where possible.
    *Compute addresses for encrypted region and key at runtime.
9-Write output binary

    *Copy original binary, apply modifications, write woody.
    *Preserve permissions and alignment.
10-Validation tests

    *Run woody and compare behavior with original.
    *Check that woody prints the banner.
    *Test with invalid inputs (non-ELF, 32-bit, corrupted) to ensure clean errors.

checklist:
    -parse
    -select region
    -encrypt
    -inject stub
    -rewrite headers
    -verify