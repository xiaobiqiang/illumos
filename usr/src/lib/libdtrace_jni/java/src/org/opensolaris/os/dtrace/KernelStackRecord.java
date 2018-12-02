/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * ident	"%Z%%M%	%I%	%E% SMI"
 */
package org.opensolaris.os.dtrace;

import java.util.*;
import java.io.*;
import java.util.regex.Pattern;
import java.beans.*;

/**
 * A value generated by the DTrace {@code stack()} action.
 * <p>
 * Immutable.  Supports persistence using {@link java.beans.XMLEncoder}.
 *
 * @author Tom Erickson
 */
public final class KernelStackRecord implements StackValueRecord,
       Serializable, Comparable <KernelStackRecord>
{
    static final long serialVersionUID = 8616454544771346573L;
    static final int STACK_INDENT = 14;
    static final StackFrame[] EMPTY_FRAMES = new StackFrame[] {};

    static {
	try {
	    BeanInfo info = Introspector.getBeanInfo(KernelStackRecord.class);
	    PersistenceDelegate persistenceDelegate =
		    new DefaultPersistenceDelegate(
		    new String[] {"stackFrames", "rawStackData"})
	    {
		/*
		 * Need to prevent DefaultPersistenceDelegate from using
		 * overridden equals() method, resulting in a
		 * StackOverFlowError.  Revert to PersistenceDelegate
		 * implementation.  See
		 * http://forum.java.sun.com/thread.jspa?threadID=
		 * 477019&tstart=135
		 */
		protected boolean
		mutatesTo(Object oldInstance, Object newInstance)
		{
		    return (newInstance != null && oldInstance != null &&
			    oldInstance.getClass() == newInstance.getClass());
		}
	    };
	    BeanDescriptor d = info.getBeanDescriptor();
	    d.setValue("persistenceDelegate", persistenceDelegate);
	} catch (IntrospectionException e) {
	    System.out.println(e);
	}
    }

    /**
     * Splits formatted call stack generated by DTrace stack() and
     * ustack() actions into tokens delimited by whitespace.  Matches
     * any number of whitespace characters on either side of a newline.
     * Can't assume that a line has no whitespace characters. A java
     * stack might have the line "StubRoutines (1)", which must not get
     * split into two tokens.
     */
    static final Pattern STACK_TOKENIZER = Pattern.compile("\\s*\n\\s*");

    /**
     * Called by JNI layer to convert a stack formatted by the native
     * DTrace library into an unformatted array of stack frames.
     *
     * @param s  string representation of stack data generated by the D
     * {@code stack()}, {@code ustack()}, or {@code jstack()} action
     * @return array of human-readable stack frames
     */
    static StackFrame[]
    parse(String s)
    {
	//
	// First trim the leading whitespace to avoid an initial empty
	// element in the returned array.
	//
	s = s.trim();
	StackFrame[] frames;
	if (s.length() == 0) {
	    frames = EMPTY_FRAMES;
	} else {
	    String[] f = STACK_TOKENIZER.split(s);
	    int n = f.length;
	    frames = new StackFrame[n];
	    for (int i = 0; i < n; ++i) {
		frames[i] = new StackFrame(f[i]);
	    }
	}
	return frames;
    }

    /** @serial */
    private StackFrame[] stackFrames;
    /** @serial */
    private byte[] rawStackData;

    /**
     * Called by native code and by UserStackRecord (in its constructor
     * called by native code).
     *
     * @throws NullPointerException if rawBytes is {@code null}
     */
    KernelStackRecord(byte[] rawBytes)
    {
	// No need for defensive copy; native code will not modify input
	// raw bytes.
	rawStackData = rawBytes;
	if (rawStackData == null) {
	    throw new NullPointerException("raw stack data is null");
	}
    }

    /**
     * Creates a {@code KernelStackRecord} with the given stack frames
     * and raw stack data.  Supports XML persistence.
     *
     * @param frames array of human-readable stack frames, copied so
     * that later modifying the given frames array will not affect this
     * {@code KernelStackRecord}; may be {@code null} or empty to
     * indicate that the raw stack data was not converted to
     * human-readable stack frames (see {@link
     * StackValueRecord#getStackFrames()})
     * @param rawBytes array of raw bytes used to represent this stack
     * value in the native DTrace library, needed to distinguish stacks
     * that have the same display value but are considered distinct by
     * DTrace; copied so that later modifying the given array will not
     * affect this {@code KernelStackRecord}
     * @throws NullPointerException if the given array of raw bytes is
     * {@code null} or if any element of the {@code frames} array is
     * {@code null}
     */
    public
    KernelStackRecord(StackFrame[] frames, byte[] rawBytes)
    {
	if (frames != null) {
	    stackFrames = frames.clone();
	}
	if (rawBytes != null) {
	    rawStackData = rawBytes.clone();
	}
	validate();
    }

    private final void
    validate()
    {
	if (rawStackData == null) {
	    throw new NullPointerException("raw stack data is null");
	}
	// stackFrames may be null; if non-null, cannot contain null
	// elements
	if (stackFrames != null) {
	    for (StackFrame f : stackFrames) {
		if (f == null) {
		    throw new NullPointerException("stack frame is null");
		}
	    }
	}
    }

    public StackFrame[]
    getStackFrames()
    {
	if (stackFrames == null) {
	    return EMPTY_FRAMES;
	}
	return stackFrames.clone();
    }

    /**
     * Called by native code and by UserStackRecord in its
     * setStackFrames() method.
     */
    void
    setStackFrames(StackFrame[] frames)
    {
	// No need for defensive copy; native code will not modify input
	// frames.
	stackFrames = frames;
	validate();
    }

    /**
     * Gets the native DTrace representation of this record's stack as
     * an array of raw bytes.  The raw bytes are used in {@link
     * #equals(Object o) equals()} and {@link
     * #compareTo(KernelStackRecord r) compareTo()} to test equality and
     * to determine the natural ordering of kernel stack records.
     *
     * @return the native DTrace library's internal representation of
     * this record's stack as a non-null array of bytes
     */
    public byte[]
    getRawStackData()
    {
	return rawStackData.clone();
    }

    /**
     * Gets the raw bytes used to represent this record's stack value in
     * the native DTrace library.  To get a human-readable
     * representation, call {@link #toString()}.
     *
     * @return {@link #getRawStackData()}
     */
    public Object
    getValue()
    {
	return rawStackData.clone();
    }

    public List <StackFrame>
    asList()
    {
	if (stackFrames == null) {
	    return Collections. <StackFrame> emptyList();
	}
	return Collections. <StackFrame> unmodifiableList(
		Arrays.asList(stackFrames));
    }

    /**
     * Compares the specified object with this {@code KernelStackRecord}
     * for equality.  Returns {@code true} if and only if the specified
     * object is also a {@code KernelStackRecord} and both records have
     * the same raw stack data.
     * <p>
     * This implementation first checks if the specified object is this
     * {@code KernelStackRecord}.  If so, it returns {@code true}.
     *
     * @return {@code true} if and only if the specified object is also
     * a {@code KernelStackRecord} and both records have the same raw
     * stack data
     */
    @Override
    public boolean
    equals(Object o)
    {
	if (o == this) {
	    return true;
	}
	if (o instanceof KernelStackRecord) {
	    KernelStackRecord r = (KernelStackRecord)o;
	    return Arrays.equals(rawStackData, r.rawStackData);
	}
	return false;
    }

    /**
     * Overridden to ensure that equal instances have equal hash codes.
     */
    @Override
    public int
    hashCode()
    {
	return Arrays.hashCode(rawStackData);
    }

    /**
     * Compares this record with the given {@code KernelStackRecord}.
     * Compares the first unequal pair of bytes at the same index in
     * each record's raw stack data, or if all corresponding bytes are
     * equal, compares the length of each record's array of raw stack
     * data.  Corresponding bytes are compared as unsigned values.  The
     * {@code compareTo()} method is compatible with {@link
     * #equals(Object o) equals()}.
     * <p>
     * This implementation first checks if the specified record is this
     * {@code KernelStackRecord}.  If so, it returns {@code 0}.
     *
     * @return -1, 0, or 1 as this record's raw stack data is less than,
     * equal to, or greater than the given record's raw stack data.
     */
    public int
    compareTo(KernelStackRecord r)
    {
	if (r == this) {
	    return 0;
	}

	return ProbeData.compareByteArrays(rawStackData, r.rawStackData);
    }

    private void
    readObject(ObjectInputStream s)
            throws IOException, ClassNotFoundException
    {
	s.defaultReadObject();
	// Make a defensive copy of stack frames and raw bytes
	if (stackFrames != null) {
	    stackFrames = stackFrames.clone();
	}
	if (rawStackData != null) {
	    rawStackData = rawStackData.clone();
	}
	// check class invariants
	try {
	    validate();
	} catch (Exception e) {
	    InvalidObjectException x = new InvalidObjectException(
		    e.getMessage());
	    x.initCause(e);
	    throw x;
	}
    }

    /**
     * Gets a multi-line string representation of a stack with one frame
     * per line.  Each line is of the format {@code
     * module`function+offset}, where {@code module} and {@code
     * function} are symbol names and offset is a hex integer preceded
     * by {@code 0x}.  For example: {@code genunix`open+0x19}.  The
     * offset (and the preceding '+' sign) are omitted if offset is
     * zero.  If function name lookup fails, the raw pointer value is
     * used instead.  In that case, the module name (and the `
     * delimiter) may or may not be present, depending on whether or not
     * module lookup also fails, and a raw pointer value appears in
     * place of {@code function+offset} as a hex value preceded by
     * {@code 0x}.  The format just described, not including surrounding
     * whitespce, is defined in the native DTrace library and is as
     * stable as that library definition.  Each line is indented by an
     * equal (unspecified) number of spaces.
     * <p>
     * If human-readable stack frames are not available (see {@link
     * #getStackFrames()}), a table represenation of {@link
     * #getRawStackData()} is returned instead.  The table displays 16
     * bytes per row in unsigned hex followed by the ASCII character
     * representations of those bytes.  Each unprintable character is
     * represented by a period (.).
     */
    @Override
    public String
    toString()
    {
	StackFrame[] frames = getStackFrames();
	if (frames.length == 0) {
	    return ScalarRecord.rawBytesString(rawStackData);
	}

	StringBuilder buf = new StringBuilder();
	buf.append('\n');
	for (StackFrame f : frames) {
	    for (int i = 0; i < STACK_INDENT; ++i) {
		buf.append(' ');
	    }
	    buf.append(f);
	    buf.append('\n');
	}
	return buf.toString();
    }
}