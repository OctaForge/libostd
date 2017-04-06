# Ranges {#ranges}

@brief Ranges are the backbone of libostd iterable objects and algorithms.

## What are ranges?

Standard C++ has an iterator system. The iterators are designed to mimic
pointers API-wise and if you need to represent a range from A to B, you need
a pair of iterators. Various APIs that work with iterators use this and take
two iterators as an argument.

However, a system like this can be both hard to use and hard to deal with in
custom objects, because you suddenly have your state split into two things.
That's why libostd introduces ranges, inspired by D's range system but largely
designed from scratch for C++.

A range is a type that represents an interval of values. Just like with C++
iterators, there are several categories of ranges, with each enhancing the
previous in some way.

You can use ranges with custom algorithms or standard algorithms that are
implemented by libostd. You can also iterate any input-type range directly
using the range-based for loop:

~~~{.cc}
    my_range r = some_range;
    for (range_reference_t<my_range> v: r) {
        // done for each item of the range
    }
~~~

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
        void pop_front();
        reference front() const;
    };

    // optional
    range_size_t<my_range> range_pop_front_n(my_range &range, range_size_t<my_range> n);
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
methods as well as some core implementations necessary for the ranges to
work. Please refer to its documentation for more information. Keep in mind
that none of the provided methods are `virtual`, so it's not safe to call
them while expecting the overridden variants to be called.

~~~{.cc}
    using range_category  = ostd::input_range_tag;
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
    range_size_t<my_range> range_pop_front_n(my_range &range, range_size_t<my_range> n);
~~~

There is one more, which is optional. This pops at most `n` values from the
range, simply going over the range and popping out elements until `n` is
reached or until the range is empty. The actual number of popped out items
is then returned. The default implementation uses slicing for sliceable
ranges, so it will be optimal for most of those. For other ranges, it just
uses a simple loop. This will work universally, but might not always be
the fastest.

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
and the backing container. When the container is unbounded, it might also
never throw. Either way, the range type is required to properly specify its
behavior. Throwing a custom exception type is a good thing because it lets
algorithms `put(v)` into ranges without checking and if an error happens
and the exception propagates, the user can check it simply as if it were
an exception thrown from the container. This makes output ranges easy to
work with, in many cases it would be otherwise very difficult to handle the
errors. Also, it makes it easy to never have to handle any errors, simply
by using output ranges backed by unbounded containers, for example
ostd::appender\_range.

### Forward ranges

Forward ranges extend input ranges. Their category tag type is obviously
ostd::forward\_range\_tag. They don't really extend the interface at all
compared to plain input ranges. What they do instead is provide an extra
guarantee - **all copies of forward ranges have their own state**,
which means changes done in one forward range will never reflect in the
other forward ranges, even if they're copies of the range. That makes forward
ranges suitable for multi-pass algorithms, unlike plain input ranges.

### Bidirectional ranges

Bidirectional ranges extend forward ranges. Their category tag type is again
pretty obvious, ostd::bidirectional\_range\_tag. They introduce some new
methods the type has to satisfy to qualify as a bidirectional range.

~~~{.cc}
    void pop_back();
    reference back() const;
~~~

Bidirectional ranges are accessible from both sides. Therefore, the new methods
allow you to pop out an item on the other side as well as retrieve it. The
actual behavior of those methods is exactly the same, besides that they work
on the other side of the range.

~~~{.cc}
    range_size_t<my_range> range_pop_back_n(my_range &range, range_size_t<my_range> n);
~~~

Obviously, as you could implement this optional standalone method previously
for the front part of the range, you can now implement it for the back. The
details for the default implementation are the same; it uses slicing for
ranges that support it and otherwise just a simple loop.

### Infinite random access ranges

Infinite random access ranges are ranges that can be indexed at arbitrary
points but do not have a known size. They extend bidirectional ranges, and
their tag is ostd::random\_access\_range\_tag. The interface extension is
very simple:

~~~{.cc}
    reference operator[](size_type idx) const;
~~~

This does exactly what it looks like. When indexing ranges, the index has
to be positive, hence `size_type`. It returns a `reference`, just like front
or back accessors.

### Finite random access ranges

Those extend infinite random access ranges. Their category tag is
ostd::finite\_random\_access\_range\_tag (duh). They extend thee range
interface some more.

~~~{.cc}
    size_type size() const;
~~~

You can check how many items are in a finite random access range. That's
not the only thing, you can additionally slice them, with this method:

~~~{.cc}
    my_range slice(size_type start, size_type end) const;
    my_range slice(size_type start) const;
~~~

Making a slice of a range means creating a new range that contains a subset
of the original range's elements. The first provided argument is the first
index included in the new range. The second argument is the index past the
last index included in the range. Therefore, doing

~~~{.cc}
    r.slice(0, r.size());
~~~

returns the entire range, or well, a copy of it. Doing something like

~~~{.cc}
    r.slice(1, r.size() - 1);
~~~

will return a range that contains everything but the first or the last items,
provided that the range contains at very least 2 items, otherwise the behavior
is undefined.

The second method takes only the `start` argument. In this case, the second
argument is implied to be the `size()` of the range. Therefore, the typical
implementation will be simply

~~~{.cc}
    my_range slice(size_type start) const {
        return slice(start, size());
    }
~~~

The slicing indexes follow a regular half-open interval approach, so there
shouldn't be anything unclear about it.

### Contiguous ranges

Ranges that are contiguous have the ostd::contiguous\_range\_tag. They're like
finite random access ranges, except they're guaranteed to back a contiguous
block of memory. With contiguous ranges, the following assumptions are true:

~~~{.cc}
    // contiguous storage
    (&range[range.size()] - &range[0]) == range.size()
    // front item always points to the beginning of the storage
    &range[0] == &range.front()
    // back item always points to the end of the storage (but not past)
    &range[range.size() - 1] == &range.back()
~~~

Contiguous ranges also define extra methods to let you access the internal
buffer of the range directly:

~~~{.cc}
    value_type *data();
    value_type const *data() const;
~~~

The meaning is obvious. They're always equivalent to `&range[0]` or
`&range.front()`.

## Range metaprogramming

There are useful tricks you can use when working with ranges and specializing
your algorithms.

### Wrapper ranges

Sometimes ranges need to act as wrappers for other ranges. In those cases, you
typically want to expose a similar set of functionality as the range you are
wrapping. So you define your category simply as

~~~{.cc}
    using range_category = range_category_t<Wrapped>;
~~~

Sometimes you can't implement all of the functionality though. What if you
want *at most* some category, but always less if the wrapped range does not
support it? For example, your wrapped range will always be at most bidirectional,
but if the wrapped range is forward it will be forward, and if it's input it
will be input. You can do that simply thanks to inheritance of category tags.

~~~{.cc}
    using range_category = std::common_type_t<
        range_category_t<Wrapped>, ostd::bidirectional_range_tag
    >;
~~~

If the wrapped range is for example random access, ostd::random\_access\_range\_tag
inherits from ostd::bidirectional\_range\_tag which is the common type for
random access range tag and bidirectional. But if it's just forward, the
common type for forward range tag and bidirectional range tag is obviously
just forward, as bidirectional inherits from it.

If you're wrapping multiple ranges and you want the capabilities of your
wrapper range to be the same as the common capabilities of all ranges you
are wrapping, you can do the same. The std::common_type_t trait takes a
variable number of type parameters.

### Category checks in algorithms

Consider you're implementing an algorithm which has a generic implementation
that works for all range categories but also a more optimal implementation
that works as long as your range is at least bidirectional. Checking by using
std::is\_same\_v doesn't quite cut it, because that will potentially disregard
"better than" bidirectional ranges, even though the algorithm is still valid
for those. You could do this:

~~~{.cc}
    if constexpr(std::is_convertible_v<
        range_category_t<my_range>, ostd::bidirectional_range_tag
    >) {
        // your more optimal algorithm implementation for bidir and better
    } else {
        // generic version for input and forward
    }
~~~

This works again thanks to tag inheritance. Any tag better than bidirectional
is convertible to bidirectional, because they inherit from it, directly or
indirectly. But it still feels unwieldy. Fortunately, custom traits are
provided by the range system.

~~~{.cc}
    if constexpr(ostd::is_bidirectional_range<my_range>) {
        // your more optimal algorithm implementation for bidir and better
    } else {
        // generic version for input and forward
    }
~~~

These break down to the same thing. Keep in mind that these never fail
to expand, so for non-range types they will become `false`.

#### Dealing with output ranges

I already mentioned above that input ranges can implement the output range
interface and become *mutable ranges*. That's why ostd::is\_output\_range
does not only check the category, but also checks the actual capabilities
of the range. So if it's possible to work with the range as with an output
range (it implements the `.put(v)` method) it will still pass as an output
range despite not having the category.

## Chainable algorithms

Input ranges provide support for implementing chainable algorithms. Consider
you have the following:

~~~{.cc}
    template<typename R, typename T>
    void my_generic_algorithm(R range, T arg) {
        // implementation
    }
~~~

You should typically also implement a chainable version. That's done by
returning a lambda:

~~~{.cc}
    template<typename T>
    void my_generic_algorithm(T &&arg) {
        return [arg = std::forward<T>(arg)](auto &&range) {
            my_generic_algorithm(
                std::forward<decltype(range)>(range),
                std::forward<T>(arg)
            );
        };
    }
~~~

Then you can do either this:

~~~{.cc}
    my_generic_algorithm(range, arg);
~~~

or you can do this:

~~~{.cc}
    range | my_generic_algorithm(arg)
~~~

This allows you to do longer chains much more readably. Instead of

~~~{.cc}
    foo(bar(baz(range, arg1), arg2), arg3)
~~~

you can simply write

~~~{.cc}
    baz(range, arg1) | bar(arg2) | foo(arg3)
~~~

This works thanks to ostd::input\_range having the right `|` operator
overloads. The implementation of the chainable algorithm still has to
be done manually though.

## Example: insertion sort

Let's implement the common insertion sort algorithm, but using range API.

~~~{.cc}
    template<typename Range>
    void insertion_sort(Range range) {
        // the type used for indexes in the given range
        using Size = ostd::range_size_t<Range>;

        // the type used for stored values in the given range
        using Value = ostd::range_value_t<Range>;

        // we're working with finite random access ranges
        Size length = range.size();

        for (Size i: ostd::range(1, length)) {
            Size j = i;

            Value v{std::move(range[i])};
            while ((j > 0) && (range[j - 1] > v)) {
                range[j] = std::move(range[j - 1]);
                --j;
            }
            range[j] = std::move(v);
        }
    }
~~~

Keep in mind that the algorithm assumes that the range uses reference-like
types for its `reference`, in order to allow writing. The values of the
range also have to satisfy `MoveConstructible`.

## Example: integer interval range

Now we'll implement an input type range that will let us iterate a half
open interval. It will be similar to the standard ostd::number\_range.

It will satisfy a forward range but won't have a writable reference type.

~~~{.cc}
    #include <ostd/range.hh>

    struct num_range: ostd::input_range<num_range> {
        // forward range, copyable with independent states
        using range_category = ostd::forward_range_tag;

        // non-templated, int type for the purpose of the example
        using value_type = int;

        // we have nothing to point refs to, the range is read only
        using reference = int;

        // unused
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        // only allow construction with specific args
        num_range() = delete;

        // our interval constructor
        num_range(int beg, int end): p_a(beg), p_b(end) {}

        bool empty() const {
            // we're empty once the beginning has reached the end
            return p_a >= p_b;
        }

        void pop_front() {
            ++p_a;
        }

        reference front() const {
            return p_a;
        }

    private:
        int p_a, p_b;
    };
~~~

That's basically it. We can now use it:

~~~{.cc}
    #include <ostd/io.hh>

    // 5, 6, 7, 8, 9
    for (int i: num_range{5, 10}) {
        ostd::writeln(i);
    }
~~~

## Example: write strings into stdout as lines

As yet another example, we'll implement an output range that writes strings
into stdout, each on a new line.

~~~{.cc}
    #include <ostd/range.hh>
    #include <ostd/string.hh>
    #include <ostd/io.hh>

    struct strout_range: ostd::output_range<strout_range> {
        using value_type      = ostd::string_range;
        using reference       = ostd::string_range &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        void put(ostd::string_range v) {
            ostd::writeln(v);
        }
    };
~~~

And usage:

~~~{.cc}
    #include <ostd/algorithm.hh>
    #include <ostd/range.hh>
    #include <ostd/string.hh>
    #include <ostd/io.hh>

    ostd::string_range arr[] = {
        "foo", "bar", "baz"
    };

    // writes foo, bar, baz, each on a new line
    ostd::copy(ostd::iter(arr), strout_range{});
~~~


## More on ranges

This is not a comprehensive guide to the range API. You will have to check
out the actual API documentation for that, start with [range.hh](@ref range.hh).
There are many predefined range types provided by that module as well as
various other APIs.
