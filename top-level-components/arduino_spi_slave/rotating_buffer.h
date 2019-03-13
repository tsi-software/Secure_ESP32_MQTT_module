/**
 * <p>File: rotating_buffer.h</p>
 * <p>Create Date: 2006-11-17</p>
 * <p>Updated:     2010-09-10</p>
 * <p>Description: Macros to implement a rotation buffer</p>
 * @author Taylor Software Inc., Warren Taylor, B.Sc.
 * @version 1.00.0
 * @copyright (c) 2006 Taylor Software Inc. All rights reserved.
 */
#ifndef ROTATING_BUFFER_H_
#define ROTATING_BUFFER_H_


/** Macros to implement a rotation buffer **/

/******************************************************************************
  E X A M P L E S
 -----------------

1)


2)


3)
    while( !is_buffer_empty( NewRfidBuffer )) {
        NewRec = buffer_read_rec_ptr( NewRfidBuffer );

        // Some initial field values.
        NewRec->RecType = 0;
        NewRec->nPopulated = 1;
        NewRec->nInvalid = 1;
        NewRec->nTxConfirmed = 1;

        // Prepare for the next buffered record.
        buffer_next_read_fld( NewRfidBuffer );
    }

******************************************************************************/


// WARNING: siz MUST be less than 255.
#define buffer_type(typ,siz)    \
struct {                        \
    unsigned char Size,         \
                  ReadNdx,      \
                  NextReadNdx,  \
                  WriteNdx,     \
                  NextWriteNdx; \
    unsigned char Overflow;     \
    typ Buffer[ siz ];          \
}

#define buffer_type_large(typ)  \
struct {                        \
    unsigned Size,              \
             ReadNdx,           \
             NextReadNdx,       \
             WriteNdx,          \
             NextWriteNdx;      \
    unsigned char Overflow;     \
    typ *Buffer;                \
}


// WARNING: this is NOT thread safe.
#define buffer_clear(buf) {     \
    buf.ReadNdx = 0;            \
    buf.NextReadNdx = 1;        \
    buf.WriteNdx = 0;           \
    buf.NextWriteNdx = 1;       \
    /* Overflow MUST be last */ \
    buf.Overflow = 0;           \
}


#define buffer_init(buf,siz) {  \
    buf.Size = siz;             \
    buffer_clear(buf);          \
}

#define buffer_init_large(buf,siz,ptr) { \
    buffer_init(buf,siz);                \
    buf.Buffer = ptr;                    \
}


// The maximum number of records that the buffer can hold.
#define buffer_size(buf)  (buf.Size)

// The size (in bytes) of a single record.
#define buffer_rec_size(buf)  sizeof(buf.Buffer[0])


//
#define is_buffer_empty(buf) ( buf.ReadNdx == buf.WriteNdx )
#define is_buffer_full(buf)  ( buf.NextWriteNdx == buf.ReadNdx )


//
#define buffer_overflow(buf)       {buf.Overflow = 1;}
#define buffer_clear_overflow(buf) {buf.Overflow = 0;}
#define is_buffer_overflow(buf)    (buf.Overflow)


// buffer_data_size(...)
// Return the number of element currently in use by the buffer.
// THIS IS NEITHER INTERRUPT NOR THREAD SAFE!
// 'rslt' should be of type 'unsigned' or 'unsigned char'.
#define buffer_data_size(buf,rslt) {                    \
    if( buf.WriteNdx >= buf.ReadNdx ) {                 \
        rslt = buf.WriteNdx - buf.ReadNdx;              \
    } else {                                            \
        rslt = buf.Size - (buf.ReadNdx - buf.WriteNdx); \
    }                                                   \
}
//
// buffer_space_remaining(...)
// Return the number of element currently unused by the buffer.
// THIS IS NEITHER INTERRUPT NOR THREAD SAFE!
// 'rslt' should be of type 'unsigned' or 'unsigned char'.
#define buffer_space_remaining(buf,rslt) {              \
    if( buf.WriteNdx >= buf.ReadNdx ) {                 \
        rslt = buf.Size - (buf.WriteNdx - buf.ReadNdx); \
    } else {                                            \
        rslt = buf.ReadNdx - buf.WriteNdx;              \
    }                                                   \
}
//----------------------------------------------------------
// Proof for buffer_data_size() and buffer_space_remaining()
//----------------------------------------------------------
// Size  WriteNdx  ReadNdx  data_size()  space_remaining()
//-----  --------  -------  -----------  -----------------
//  5       0         0      0 - 0 = 0    5 - (0-0) = 5
//  5       2         0      2 - 0 = 2    5 - (2-0) = 3
//  5       4         1      4 - 1 = 3    5 - (4-1) = 2
//  5       1         2     5-(2-1) = 4    2 - 1 = 1
//  5       2         4     5-(4-2) = 3    4 - 2 = 2
//  5       3         2      3 - 2 = 1    5 - (3-2) = 4
//  5       4         4      4 - 4 = 0    5 - (4-4) = 5
//  5       4         0      4 - 0 = 4    5 - (4-0) = 1
//----------------------------------------------------------


//-------------------------------------------------------------------------
// Don't call these two macros directly. They are used by the macros below.
//-------------------------------------------------------------------------
// TODO: this needs to be made multi-thread / interrupt safe.
#define _buffer_next_write(buf) {               \
    buf.WriteNdx = buf.NextWriteNdx;            \
    buf.NextWriteNdx++;                         \
    if( buf.NextWriteNdx >= buffer_size(buf) ){ \
        buf.NextWriteNdx = 0;                   \
    }                                           \
}

// TODO: this needs to be made multi-thread / interrupt safe.
#define _buffer_next_read(buf) {                \
    buf.ReadNdx = buf.NextReadNdx;              \
    buf.NextReadNdx++;                          \
    if( buf.NextReadNdx >= buffer_size(buf) ) { \
        buf.NextReadNdx = 0;                    \
    }                                           \
}
//-------------------------------------------------------------------------


// WARNING:
// you must test is_buffer_full() before calling buffer_write().
// TODO: this needs to be made multi-thread / interrupt safe.
#define buffer_write(buf,p) {       \
    buf.Buffer[buf.WriteNdx] = p;   \
    _buffer_next_write(buf);        \
}


// TODO: this needs to be made multi-thread / interrupt safe.
#define buffer_safe_write(buf,p) {  \
    if( is_buffer_full(buf) ) {     \
        buffer_overflow(buf);       \
    } else {                        \
        buffer_write(buf,p);        \
    }                               \
}


// WARNING:
// you must test is_buffer_empty() before calling buffer_read().
// TODO: this needs to be made multi-thread / interrupt safe.
#define buffer_read(buf,p) {        \
    p = buf.Buffer[buf.ReadNdx];    \
    _buffer_next_read(buf);         \
}


// WARNING:
// you must test is_buffer_empty() before calling buffer_peek().
// TODO: this needs to be made multi-thread / interrupt safe.
// Get the value but do NOT  move to the next node.
#define buffer_peek(buf) ((buf).Buffer[(buf).ReadNdx])


//-------------------------------------------------------------------------
// The following macros allow 'Buffer' to be a struct.
// Do NOT use these macros if your buffer is an atomic type (e.g. int).
// Likewise, do NOT use the above Read and Write macros if your buffer
// type is a struct.
//-------------------------------------------------------------------------

// TODO: these need to be made multi-thread / interrupt safe.
#define buffer_write_fld(buf,fld,p) {   \
    buf.Buffer[buf.WriteNdx].fld = p;   \
}

// Uncomment this if it is needed.
//#define buffer_write_fld_ptr(buf,fld,p) { \
//    buf.Buffer[buf.WriteNdx]->fld = p;    \
//}

// Use these next two macros as little as possible!!!
// The are only intended for use with functions such as memcpy().
// e.g.
//  memcpy( buffer_write_rec_ptr(buf), ptr, REC_SIZE );
//  memcpy( buffer_write_member(buf,rfid), ptr, FLD_SIZE );
#define buffer_write_rec_ptr(buf)     (&(buf.Buffer[buf.WriteNdx]))
#define buffer_write_member(buf,fld)  (buf.Buffer[buf.WriteNdx].fld)

// WARNING:
// you MUST test is_buffer_full() before calling buffer_next_write().
// TODO: this needs to be made multi-thread / interrupt safe.
#define buffer_next_write_fld(buf) {  \
    _buffer_next_write(buf);          \
}

// Uncomment this if it is needed.
// TODO: this needs to be made multi-thread / interrupt safe.
//#define buffer_next_safe_write_fld(buf) {   \
//    if( is_buffer_full(buf) ) {             \
//        buffer_overflow(buf);               \
//    } else {                                \
//        buffer_next_write_fld(buf,p);       \
//    }                                       \
//}

// WARNING:
// you must test is_buffer_empty() before calling buffer_read_*().
// TODO: these need to be made multi-thread / interrupt safe.
#define buffer_read_fld(buf,fld,p) {    \
    p = buf.Buffer[buf.ReadNdx].fld;    \
}
// Uncomment this if it is needed.
//#define buffer_read_fld_ptr(buf,fld,p) {  \
//    p = buf.Buffer[buf.ReadNdx]->fld;     \
//}

// Use these next two macros as little as possible!!!
// The are only intended for use with functions such as memcpy().
// e.g.
//  memcpy( ptr, buffer_read_rec_ptr(buf), REC_SIZE );
//  memcpy( ptr, buffer_read_member(buf,rfid), FLD_SIZE );
#define buffer_read_rec_ptr(buf)     (&(buf.Buffer[buf.ReadNdx]))
#define buffer_read_member(buf,fld)  (buf.Buffer[buf.ReadNdx].fld)

// WARNING:
// you must test is_buffer_empty() before calling buffer_next_read().
// TODO: this needs to be made multi-thread / interrupt safe.
#define buffer_next_read_fld(buf) {  \
    _buffer_next_read(buf);          \
}


#endif // ROTATING_BUFFER_H_
