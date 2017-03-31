# Ranges {#ranges}

@brief Ranges are the backbone of OctaSTD iterable objects and algorithms.

## What are ranges?

Standard C++ has an iterator system. The iterators are designed to mimic
pointers API-wise and if you need to represent a range from A to B, you need
a pair of iterators. Various APIs that work with iterators use this and take
two iterators as an argument.

However, a system like this can be both hard to use and hard to deal with in
custom objects, because you suddenly have your state split into two things.
That's why OctaSTD introduces ranges, inspired by D's range system but largely
designed from scratch for C++.

A range is a type that represents an interval of values. Just like with C++
iterators, there are several categories of ranges, with each enhancing the
previous in some way.

## Implementing a range

Generally, there are two kinds of ranges, *input ranges* and *output ranges*.
*Input ranges* are ranges you read from and *output ranges* are ranges you
write into. There are several categories that extend input ranges, namely
*forward ranges*, *bidirectional ranges*, *infinite random access ranges*,
*finite random access ranges* and *contiguous ranges*. These actually more
or less map to the corresponding iterator categories. You can also have any
input range meet the requirements for an output range. These are called
*mutable ranges*, similarly to C++ *mutable iterators*.

### Characteristics of an input range

~~~{.cc}
    #include <ostd/range.hh>

    struct my_range: ostd::input_range<my_range> {
        using range_category  = ostd::input_range_tag;
        using value_type      = T;
        using reference       = T &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        my_range(my_range const &);
        my_range &operator=(my_range const &);

        bool empty() const;
        bool equals_front() const;
        void pop_front();
        reference front() const;

        // optional methods with fallbacks
        void pop_front_n(size_type n);
    };
~~~

This is what any input range is required to contain. An input range is the
simplest readable range type. The main thing to consider with simple input
ranges is that if you make the copy of the range, it's not required to be
independent of the range it's copied from. Therefore, a simple input range
cannot be used in elaborate multi-pass algorithms, but it's useful for stuff
like ranges over I/O streams, where the current state is determined by the
backing stream for all ranges that point to it and thus all ranges change
when the stream or any of the ranges do.

Ranges typically don't store the memory they represent a range to. Instead,
they store a reference to some backing object and therefore their lifetime
relies on the backing object. This does not have to be the case though,
there can also be range types that are completely independent, for example
ostd::number\_range. But typically it is the case.

But let's take a look at the structure first.

~~~{.cc}
    struct my_range: ostd::input_range<my_range>
~~~

Any input range (forward, bidirectional etc too!) is required to derive
from ostd::input\_range like this. The type provides various convenience
methods as well as fallbacks for optional methods. Please refer to its
documentation for more information. Keep in mind that none of the provided
methods are `virtual`, so it's not safe to call them while expecting the
overridden variants to be called.

~~~{.cc}
    using range_category = ostd::input_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
~~~

Any input range is required to have a series of types that define its traits.

The `range_category` alias defines the capabilities of the range. The possible
values are ostd::input\_range\_tag, ostd::forward\_range\_tag,
ostd::bidirectional\_range\_tag, ostd::random\_access\_range\_tag,
ostd::finite\_random\_access\_range\_tag and ostd::contiguous\_range\_tag.

The `value_type` alias defines the type the values have in the sequence. It's
not used by plain input ranges, but it can still be used by algorithms and
it's used in more elaborate range types.

The `reference` alias defines a reference type for `value_type`. It's what's
returned from the `front()` method besides other places. Keep in mind that it
does not necessarily have to be a `T &`. It can actually be a value type, or
for example ostd::move\_range represents it as an rvalue reference. So it
really is just a type that is returned by accesses to the range, as accesses
are typically not meant to be copy the contents (but they totally can).

The `size_type` alias represents the type typically used for sizes of the
range the object represents. It's typically `size_t`. Similarly, the
`difference_type` alias is used for a distance within the range. Usually
it's `ptrdiff_t`, but for example for I/O stream range types it can be the
stream's offset type.

Now let's take a look at the methods.

~~~{.cc}
    my_range(my_range const &);
    my_range &operator=(my_range const &);
~~~

Any range type is required to be *CopyConstructible* and *CopyAssignable*.
As previously mentioned, this does not have to mean that the internal states
will be independent. With input ranges, it can all point to the same state.
From forward ranges onwards, independence of state is guaranteed though.

~~~{.cc}
    bool empty() const;
~~~

This method checks whether the range has any elements left in it. If this
returns true, it means `front()` is safe to use. Safe code should always
check; the behavior is undefined if an item is retrieved on an empty range.

~~~{.cc}
    bool equals_front() const;
~~~

This checks whether the front part of the range points to the same place.
It doesn't do a value-by-value comparison; it's like an equality check for
the range but disregarding the ending of it. Typically this returns true
only if the front part of the ranges point to the same memory.

~~~{.cc}
    void pop_front();
~~~

This turns a range representing `{ a, b, c, ...}` into `{ b, c, ... }`, i.e.
removes the first item from the range. The item typically still remains in the
backing object for the range; but as an input range potentially modifies the
internal state of its backing memory, it doesn't have to be the case. It's
always the case for forward ranges onwards though.

Calling `pop_front()` is undefined when the range is empty. It could throw
an exception but it could also be completely unchecked and cause an invalid
memory access.

~~~{.cc}
    reference front() const;
~~~

And finally, this retrieves the front item of the range, typically a reference
to it but could be anything depending on the definition of the `reference`
alias. Calling this is undefined when the range is `empty()`. It could be
invalid memory access, it could throw an exception, it could blow up your
house and kill your cat. Safe algorithms always need to check for emptiness
first.

~~~{.cc}
    void pop_front_n(size_type n);
~~~

There is one more, which is optional. This pops `n` values from the range.
It has a default implementation in ostd::input\_range which merely calls the
`pop_front()` method `n` times. Custom range types are allowed to override
this with their own more efficient implementations.

### Output ranges

Output ranges are a different kind of beast compared to input ranges. I could
cover them at the end but I'll do it now instead. This is the structure of
an output range:

~~~{.cc}
    #include <ostd/range.hh>

    struct my_range: ostd::output_range<my_range> {
        using value_type      = T;
        using reference       = T &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        my_range(my_range const &);
        my_range &operator=(my_range const &);

        void put(value_type const &v);
        void put(value_type &&v);
    };

    // optional
    template<typename IR>
    void range_put_all(my_range &output, IR input);
~~~

As you can see, they're much simpler than input ranges.

~~~{.cc}
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
~~~

Why is there no `range_category` here? Well, it's already defined by the
ostd::output\_range it derives (and has to derive) from. We already know that
it will always be ostd::output\_range\_tag. Might as well avoid specifying it
always.

Output ranges are always copyable, just like input ranges. There are no rules
on state preservation.

~~~{.cc}
    void put(value_type const &v);
    void put(value_type &&v);
~~~

This will insert a value into the output range. Typically, it will trigger some
writing into a backing container. There are no guarantees on how the position
will be affected in other input ranges. Most frequently, all output ranges
pointing to a container of some sort will just do some kind of append.

~~~{.cc}
    template<typename IR>
    void range_put_all(my_range &output, IR input);
~~~

This optional method has a default implementation in the library which simply
goes over the `input` and calls something like `output.put(input.front())` for
each item of `input`. You're free to specialize this (argument-dependent lookup
is performed by library calls to this) with a more efficient implementation if
you wish.

Additionally, any input range is allowed to implement the output range interface.
If the library ever does any checks for whether the given range is an output
range, it will do a check **based on capabilities of the range** rather
than just its category. Input ranges that implement output range interface are
called *mutable* input ranges. Most frequently, their `r.put(v);` will be the
same as `r.front() = v; r.pop_front();` but that is not guaranteed.

Additionally, `put(v)` is always well defined. When it fails (for example
when there is no more space left in the container), an exception will be
thrown. The type of exception that is thrown depends on the particular range
and the container it backs. When the container is unbounded, it might also
never throw. Either way, the range type is required to properly specify its
behavior. Throwing a custom exception type is a good thing because it lets
algorithms `put(v)` into ranges without checking and if an error happens
and the exception propagates, the user can check it simply as if it were
an exception thrown from the container. This makes output ranges easy to
work with, in many cases it would be otherwise very difficult to handle the
errors. Also, it makes it easy to never have to handle any errors, simply
by using output ranges backed by unbounded containers, for example
ostd::appender\_range.
