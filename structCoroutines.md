In C, notice that the legal struct bodies are a subset of the legal function bodies. That is, a struct and a function can both be composed of several declarations.

In my language, the main change is that the legal struct bodies are now the full set of legal function bodies. A struct is essentially a standardization for retrieving a function's stack frame after it runs. This allows the prior use case completely, but enables structs to narrate their lifetime using labels, yields, and reentrancy.

pseudomembers are a blanket category for static data about a type or value. Operators like `typeof` or `sizeof` from C are generally pseudomembers here. Certain actions are also pseudomember concerns. `_` is the pseudomember resolution character; snake case is forbidden.

# Declaration

The platonic form for defining a struct type is `struct name {};`. After, `struct` is not a necessary prefix to `name` in declarations; it is used by itself, like in C++.

The other forms of struct definition/declaration are:

`name {};`, where `struct` is implicit. There are a few extra `struct` types, so this default is important.

`struct {} name;`, which creates a single instance of a struct with the given body. Its type is not lost; it is a pseudomember `typeof` for `name`. `name_typeof` allows you to get the type back later if you want.

`{} name;`, like above.

`struct x_typeof name;` typedef-like for an already-declared anonymous variable.

Structs support compound initializers, with the language allowing the initialization constants to fully apply before running the struct's function. This means the "winner" (initializer or function) is clearly defined; the initializer does its thing and then the function does. Compound initializers do not require an `=`:

Setting a struct with a compound member literal does not modify members that were not named. This can be done after initialization.

```
    type myVal{.x=0, .y=1, ...};
```

Statements are generally much more expression-like than typical languages, though this is not a blanket rule. Declarations can return values, so a declaration is an expression.

Normally, after a type name, a declaration only accepts the name of the declared value. Here, declarations can support a struct's members being declared into the local scope via comma-separated names. `T (name0, name1);` works on its own if the type contains members with the same names. If you want to alias them further, `type (name0=member0, name1=member1);` works. If you also want a handle to the greater struct, put its name first and the remainder in parentheses: `type myStruct(name0=member0, name1=member1);`. This is useful for structs that are used in a more function-like fashion (more than likely `frame` types) if you want to derive several values from its instantiation and then forget the base object. Doing it this way lets the compiler optimize space more easily. It's also useful for if you prefer to unpack a single struct's names into the local scope, so that you aren't typing the struct name repeatedly.

A struct variable declaration can omit a name in order to run that type's init routine without actually creating a declaration from it: `structType{initializer};` or indeed just `structType;`. Either of these may return a value.

As mentioned, an initializer can be tagged onto a `struct@label` call to set members before the call is executed (`struct@label{initializer}`).

Positional (unnamed) initializer arguments are put into a struct's members in order by their order of declaration after the label. This is not a deviation from expectation in the base case; a declaration calls the "label" at the very start of the struct (which exists whether it is declared or not), after which declarations follow in order. This mirrors C, where struct initializers file into the struct in declaration-order. In my language, the same is done; except that the base location starts after a given label. If that label is the implicit start, it's exactly like C.

This can be a little touchy, and a few safety warnings apply:

If you pass first to any number of `param`-type members (explained later), but then move on to `member` or `temp`-type, the compiler should warn you.

If you pass a mix of `.name=` and positional members, this is legal but should be warned about (perhaps with an aggressive, non-default warning standard). `.name=` members do not affect the order of positional members. A `.name=` pauses the evaluation of the positional arguments, sets whatever it sets (even if the same member *is* set positionally at any point), and resumes. Members are well-definedly processed left to right. named and positional arguments both setting the same member can be a separate warning which probably *should* be default.

# Storage and memory layout

`temp` is a storage qualifier that prevents a variable from necessarily being stored in the final struct after execution returns to the caller.

Members are defined to be placed in-order in a struct, except that a temp member's storage goes after all non-temp members and is undefined beyond that. The compiler is free to make `temp` members exist in a more abstract sense, such as at remote memory offsets or in registers.

`temp` has an opposite, `member`. This is the default for all `struct` definitions. A `frame` is a struct for which `temp` is the default and `member` must be specified. 

A struct can be declared `full`, in which case it must reserve space for even its temp members. The compiler is free to internally promote a struct to full if doing so does not prohibit alignment and offset guarantees, but it will still require the struct to be labeled full if `temp` members are accessed outside where they were necessarily set.

When a struct is copied by value, full struct -> struct drops any temp members.

The compiler must analyze execution to create boundaries and understand declarations. Statements that affect control flow and frame layout are treated skeptically in the way that makes the most cautious assumption; if a `return` may run conditionally, analysis concludes that it must not for the sake of dividing execution into segments that can see declarations and allocating the right amount of frame space. In other words, `if (condition) return;` is considered to have never run when dividing the function into segments that can or cannot see temp declarations. Declarations themselves are the opposite; if a value *may* be declared, it *has* been.

Because there are no guarantees about how and where temp members are stored, the compiler is free to analyze structs that are declared full and only reserve storage for the members that are actually used.

Nested blocks inside struct bodies are implicitly `frame`-type. Their members are added to the upper scope, but default to `temp` because the block is a `frame`. If they shadow a higher definition, they are not accessible outside that block. Accessing members outside a nested block may be a compiler warning.

The caller can manually set `temp` members, and doing so reserves space in the caller's frame. The struct will then be in between regular and full; structs can be thought of as one region of defined storage, and a trailing region of undefined storage. A function body must be scanned for expressions that set members before allocating storage for them, unless it uses the simple case and promotes a struct to full behind the scenes. A struct in an array cannot have any temp members unless the whole array is declared to contain full structs.

To restate the above, a caller can refer to a struct's `temp` member *IF*:
* It is explicitly set *by* the caller (reserving storage)
* The whole struct is declared as `full`

What these cases have in common is that the caller allocates space for the `temp` member.

Knowledge of whether a `temp` has caller space reserved is an ABI concern, as a struct's label needs to know whether a `temp` will be lost *in between* calls. ABI concerns do not materialize until the callsite.

A label call *generally* fails at compile time if a `temp`'s value is used, but it was declared in a separate label call. However, because ABI concerns do not materialize until the callsite, the caller may enable the label call by having storage for that `temp` member.

Redeclaring member names across label boundaries is okay if the originals were declared `temp`. The originals remain accessible as pseudomembers for the `struct` via `struct_labelname.shadowedMember`, where `labelname` is the label under which a name was declared. For compound initializers, `.name=value` refers to the label being initialized for.

`param` is a derivation from temp which enforces that label calling fails if the callsite cannot see the param-declared members the label uses being directly set. `param` must not just have known storage (i.e. it implicitly has storage via `full`); it must be unconditionally set to a value in the calling context. `param` is also a hint to the compiler: param members are much less likely to actually need to be stored permanently. It can reserve temporary space for them rather than permanent space on the caller.

With the simple version out of the way, `param` is actually slightly more complex. It is both a declaration qualifier and a semantic statement. Declaring a variable as `param` makes it `temp` plus setting guarantees. It can be used as an alias for `temp` *unless* it is followed by a contradiction (`param member int x;`). Contradictions are respected; they simply say "do not use `param` as an alias for `temp` for this declaration. Using it as a statement `param already-declared-var` temporarily marks that variable as a parameter for the label it's in. This lets you have an already-declared member, but enforce that the caller provides a new value for it.

`ret` is the final such derivation. This allows the programmer to declare a member for returning and get an error when the set of `ret` declarations is disobeyed. `ret` and `param` are both local to a label. A label with *no* `ret` declarations inside is okay with returning any member (including no member at all). A label with any nonzero number of `ret` declarations inside is unwilling to return anything other than the set of `ret`-declared members (including nothing). `ret` declarations follow `param` declarations almost exactly. They imply `temp` unless overridden, and `ret` can be used to apply context to an existing member name.

All above qualifiers can precede a block (including sensical combinations like `param member`), inside which block the defaults are overridden.

Where a statement or compound statement may go, `*;` means the same thing as "the remainder of my parent compound-statement is nested inside of me". It's equivalent to wrapping the remaining block scope in a set of braces.

You can effectively redfine the defaults for an entire scope by saying `qualifier *;`

Because default redefinition is so easy, `struct` vs. `frame` is mainly useful as a declaration of intent rather than an ergonomics feature. `frame` means "should generally run once and be forgotten; maybe hold a reference to it in case a yield needs to be negotiated"; `struct` means "should be held on to as an entry point for several operations on related data".

From a compiler perspective, structs should be thought of differently in this language than in others. In this language, a struct *does* have layout properties, but the language's default model is *not* to respect that layout. The programmer may still reason about structs as if they *were* run-of-the-mill, as the underlying system is designed to guarantee correct struct behavior. The only programmer concern is that they should feel freer about declaring structs whose members they do not use, especially if a struct declaration is treated as a function call.

There are infinite type derivations given a single struct. Essentially, each block of code is willing to believe that any type name in a declaration exists, and will begin to build a notion of it as the declared value as it parses. If it sees a `.member` access and it hadn't seen one before, it will update its notion. A notion of a type can generate conflicts *before* knowing its true definition, which causes a failure. For example, no type allows both `+` and `.member`. If such a conflict is found, that is a compilation failure point. Each isolated block of code generates its own notion, and the greater parsing context collects an aggregate notion, failing at *any* conflict.

A declaration of a type then checks against all built notions. If any one of them is invalid according to the definition, compilation fails. Any members that a notion declared but that do not exist; any members which were used as one type but are not of that type; etc. Some notions can be correct for more than one type, so the declaration simply corrects those if it can (i.e. a member could've been either an `int` or a `float` based on usage, but it's declared as a `float`, so the notion updates accordingly). Note that declarations *should* go before all usages so that improper notions can be caught early.

Note that while this model is a platonic view of what happens, its behavior is *always* identical to a typical declaration-first struct system in other languages. Declaration *should* precede any use of a type, which allows the compiler to check in real time while *also* building a private notion. The compiler should warn when it has to build a notion before it sees a concrete declaration. In fact, it could even error; explaining notions as if they are fulfilled by a *future* definition is just a good way to get an idea of what they are. 

The power of notions is that after a struct verifies them, it *doesn't* have to respect the global declaration. If it keeps the struct private and doesn't use any of the layout-guaranteed features of a struct, it can maintain its private copy of the definition without issue. If it passes the value to another known scope, it can actually cooperate with that scope. It may still not need to use the canonical declaration; it may simply be able to translate from one notion to the next. Only when passing a struct to an opaque context or when using layout guarantees does it need to obey the global definition. In that case, a block can either use the global definition from the start or translate to a global copy at the callsite.

# calling, returning, yielding, and `label`-type variables

`return` takes either a member name or an assignment to a member name as its operand: `return val = 0;` or `return val;`. This is required because it defines a callsite acceptor. Essentially, returning must specify a member involved in the return; no uncontained expression may be used. The ABI involves the callsite slightly more, requiring that the callsite knows which struct member to treat as the return value. This can be done via pointer passing if inlining and static analysis are not available or sufficient. The callsite, if it accepts a returned value, must verify that the path it can take never returns a different type.

`label` is a type of value which is used for storing dynamic entry points.

`yield` is a pseudomember of any instance of `label` type, and it performs an action. `myLabel_yield expr` is a specialized `return` that stores the execution state inside `myLabel` and then returns `expr` with the same rules as regular `return`. The definition of the execution state is undefined, but the programmer may reason about it as if it stores only one scalar value. It can require additional data if certain static data is lost, but that case requires specific programmer intervention (more on this later).

The difference between `labelVar_yield` and `return` is simply that `yield` stores an exact re-entry point, whereas `return` requires the caller to know which static label to call next if it wants to re-enter.

Inside a struct's definition, the type is only ever called `label`, but it is a unique and mutually-incompatible instance of the type per-struct. The private label type is accessible from outside as a pseudomember, `_label`. A `label` variable can be set using enumerated constants corresponding to the labels declared inside a struct. When assigning to a `label` type variable, it has a private namespace of constants it will accept, which namespace is defined by the labels declared in its owner. `label`-type variables can also be set using `label yield value` statements inside a struct's execution. `yield` and `label`-type variables are used for unstructured reentrancy: yielding routines that might want to exit for reasons that don't have to do with the control flow structure. An example might be a call that yields because it knows an operation would block. When writing structured reentrancy, or yielding routines that exit for explicit control flow reasons, yield and label variables are not necessary. `yield` allows state machines that have a single sentinel for remembering the state to return to, but often this is overkill and builtin context can be used to dispatch state transfer manually.

`@` is a binary operator. Outside a struct definition, `structInstance@label` re-enters a struct's definition after it has returned. Inside, stack@label pushes the current frame and jumps. In any case, obj@label{.member = n, ...} applies the inner changes after storing the old state if applicable. In the external call case, this doesn't apply, as there is no frame-switching. The return result of structInstance@label is an inferred member of that struct.

`label` may be a `labelname:` defined inside the struct, or it may be a `label x;` declared-member.

One `label` per struct can be qualified `main`. If there is a `main` label, `yield` does not need its lhs; it assumes the main label. Returning to a `label` variable after it has stored a value requires `struct@labelvarname`.

Basic structs can just initialize their members and yield, place a delete label, and de-initialize after that label. A single function then narrates a struct's complete lifecycle, broken up by labels.

A reference to the entrypoint for a struct's execution is accessible via `type_init` `instance_init`. `instance@instance_init`, for example, calls the entrypoint, even if there is no explicit label there. Type-based pseudomembers are generally granted to instances of the type.

# labels and pattern matching

Labels have a notable behavior: They do not fall through by default as in C and many others, but can be declared as either `boundary` or `fallthrough`. `boundary` labels implicitly return void when reached (or `break` if nested), and they are the default type of label in most contexts. `case` labels are not special, meaning switch statements don't need a `break;` after every case which is meant to.

`fallthrough` and `boundary` may also precede an enclosing block (which then may be replaced by a `*;`.

A block may use constant values as labels rather than names. The constant values can be enum values. A block can *only* have either symbolic labels or constant labels of a single type, except that nested blocks may deviate from their parent, and deviating labels are not owned by their parent. `@` applies to both struct instances and arbitrary code blocks. A switch construct is a block with constant labels, entered via `@` or `->`. `@` and `->` are essentially the same operator with switched order. `struct@label` says "call struct with symbol 'label'". `label -> struct` does the same thing, but is expressed more like "use this label as a key to struct". `struct` is interchangeable with any block that contains labels, and `label` is interchangeable with a value to be matched to a label. They could be more generically be called a `key` and a `target`. Keys are label-like, and targets are function-like. A block that is treated as a `target` (by virtue of being an operand for either operator) does not execute unconditionally (as a block typically should); it executes only with the given key.

`void` is a constant that agrees with a `void` return.

`default` is a constant that agrees with anything after all other labels.

The `virtual` label lets callers treat a value like a label, while the callee recieves the passed-item as a parameter. From the outside, it can look like a struct exhaustively handles every label for a given constant type; inside, it is handled generically. A `virtual` label declaration is:

`virtual(declaration):`

The `declaration` is the acceptor for the passed-in label. It is declared as a `param`-type member for the struct.

`break` is like `return` for a block that is nested in a parent block, meaning it accepts an expression for returning as well (with the same requirements as `return` has).

# weakly typed values

A callsite that can return any of several members from a struct evaluates to a type of value for whom no operation is valid except checking equality and limited member access. It is called `undef`, or `myStruct_undef` as a pseudomember of a struct. Under the ABI, it is a symbolic constant representing which member was set by the return or yield. A block that is the rhs of `struct@label -> {}` is privileged to get access to special constants that correspond to the different possible values. For each member that label may return, `.member` is a constant corresponding to that case. A little more generally, the `undef` return type, when involved with comparisons, has access to a special private namespace that includes `.member` for each member it may represent.

The `undef` return type can be accessed as any one of its potential members using a `.` access. This potentially produces a special value. If that specific member was set, the value is simply that member. However, if not, it becomes a special value that nullifies the entire expression it is in, so that no side effects can be had. member-accessing the `undef` result of a label call has the highest possible precedence, and it always evaluates left-to-right. Any accesses that may result in the nullifier are evaluated before the entire remaining expression, left-to-right, and the first non-set factor prevents the entire remainder from executing. Therefore, side effects only occur if the leftmost `n` labelcall accesses succeed and then another does not. It is best practice to use only one per expression so no surprises are had. Note that assignment is a side effect of an expression; therefore, if a variable is assigned to an expression containing a member access, and that member access fails, no assignment occurs at all: the variable retains its value.

The nullification also applies to statements; statements such as `return` will not execute if an access they depend on fails. This lets you handle multi-type returns as a sort of "multiverse of possible returns" where several return statements are in a row, but at most one of them can actually run. The condition is contained inside whether the `.member` was set.

`with(value) {}` is a control flow keyword used specifically to gatekeep multiple statements behind a possibly-nullifier value. It does nothing except evaluate the expression inside.

# blocking

// TODO: blocking (a way to wrap sequences of calls into a contract where the callee can signal "would block" by returning via a particular member)

# try/except

// TODO: try/except (a way to wrap sequences of calls into a contract where the callee can signal "failure" by returning via a particular member)

# tagging

This language uses a few features which rely on the compiler's static knowledge and degrade when that is lost. The `undef` type is not such an example, because its canonical definition is to return an identity for the actual value. The optimized case, where the compiler knows which value was returned and inlines a jump to the exact case handling it, should simply be more common than the canonical case.

The has a two-part contract for negotiating runtime tagging when the compiler sometimes can provide a property statically and other times cannot.

First, properties may be treated as if they are known. The variety of methods for querying associated data from a value (which should require additional storage unless they are statically known) can be treated as if the runtime simply knows their value. The compiler will then run a set of defined static analyses involved with inlining (which will not be implementation dependent) and simply provide those values if it can. If it cannot, it will raise an error. Errors happen at instantiation sites, not definition sites. A callsite has privilege to information that may help it with static information. An important property of this static information is that it requires a very well-defined procedure for discovery. This is two-pronged. The definition of the language should not require egregiously deep analysis in order for a compiler to be valid; this would both raise the barrier to implementation and make compile times necessarily quite slow. Additionally, making the process for determining static qualities very deep will quickly outpace the programmer's own reasoning about whether the compiler can see static guarantees. Procedure calling can potentially become a minefield of not knowing whether the compiler will complain. The procedure for discovering static data should be reasonably in-reach of the programmer's own ability to do so. The compiler is free to continue to analyze *after* it completes the defined procedure, but it may not use that analysis to allow something that could be prohibited with another compiler's optimization strategy. It is reasonable, however, that the compiler may provide an attribute asking it to go beyond the standard and allow anything it can prove.

If the compiler determines that a property cannot be known at a given callsite, or (the main case) if a handle to an execution context is passed to another context that cannot confidently inline, the compiler will error, as mentioned above. To resolve this, the members that require static data must be qualified by `tagged`. This is a programmer hint to the compiler that it is allowed to associate extra data with the variable as needed. If the caller must handle the discrepancy instead, it can use the `tag` keyword to annotate an operation as being free to store additional data.

There may be specializations of `tagged` that accommodate *only* one instance of static data.

# `generic` type

A value may be declared without a known type as `generic`. `generic` is quite similar to `undef`, except that it does not emit the nullifier. A `generic` has a member for every type. It (along with all other types) also has a pseudomember `type` which can be used to get a compiler-defined, guaranteed-unique identity. `generic.type` interprets the bytes of `generic` as the given type, whether it was set to be that type or not. For safe, tagged handling, use the `union` pseudomember. `genericInstance_union` *produces* an `undef`-typed value.

If the `generic`-typed value cannot have its type proven statically, it must be `tagged`. Leaving a `generic` untagged is a statement of the intention that a procedure should only be invoked in inlinable contexts.

`generic` cannot participate in the `temp/member` system quite as simply as other types. If it wants to be tagged as `member`, it must also be tagged `fixed`. `fixed` implies `member` unless overridden. This requires the caller for a struct to instantiate that struct strictly for accepting one type of value and fail if a different type is ever used.

A `generic` variable can be redeclared in the context of a following block. That is, if there is a `generic` value named `x`, I may simply redeclare it and follow it with a compound statement in which `x` refers to the integer interpretation of the generic value.

```
    generic param x;
    // unsafe but allowed: we have not proven `x` holds an `int` type
    int x {
        printf{"x: %d\n", x};
    }
```

This pairs nicely with a `*;`, redeclaring a `generic` inside the remainder of a scope.

```
    generic param x;
    // guard against all but `int` values for the remaining scope and redeclare `x` as an `int`
    with (x_union.int) int x *;
    printf{"x: %d\n", x};
```

# variadic arguments

This language has no variadic arguments, as it has no arguments at all. Instead, it uses a negotiation strategy between callee and caller. Placing `...` where an argument is expected initiates the negotiation. Everything following `...` participates in the procedure regarding the parameter.

For each argument that `...` consumes, the label in question is called again, with the same member set to the next value. You can quarantine the domain of `...` using parentheses. Values must agree with the type of the member being set, meaning `generic` `...` sequences can accept arbitrary values. The separate calls to a variadic-negotiated label are privileged to retain all data, even `temp`.

# external labels

As was mentioned, `label` is a private type for which each `struct` has an instance. The specific `label` type for a struct is given as a pseudomember, providing a way for an external context to retain a callback-like handle to a specific label without knowing what it is or where it came from. Since there are neither return types nor parameters, a `label` has no type information beyond the struct type it came from. This must often also be erased, casting a `myStruct_label` type to a `generic_label` type.

A `label` cannot run without a base struct. Additionally, the `label` and the `struct` must agree in type. In order for this to be type safe, the base struct must be a property of a label. Specifically, the base struct is a property of a `tagged` label. A `label` can be executed in terms of the struct it was derived from by simply omitting the base from the call. `@someLabel` calls that label via its base if the base is known either statically or via an internal tag.

Bar using the `type` tag of a `generic_label` type, params cannot be set for the base object. Therefore, in contexts that do not engage in runtime polymorphism, a `generic_label` can only be dispatched as a closure over its context. Since the value a label returns is known by also knowing its type and checking against an enumeration specific to that type, a `generic_label` can produce no return value. It still may, however, be used to trigger an effect.

In order to engage with a `generic_label` via the ABI, a struct must interact with the `interface` system, whose details I have not fully worked out. The `interface` system will be a way for a `struct` to partially define itself in terms of a common layout so that callers can dispatch statically without knowing its initial type.

// TODO: fully explain `interface` system

# interfaces (not complete, brief notes)

The language should define a few builtin interfaces which allow structs to participate in runtime behavior without necessarily having a specific set of inner names. These will have to do with resuming in compiler-managed reentrancy negotiations (as above), scheduling/awaiting, initialization and destruction, and a few other language constructs. The builtin interfaces may or may not share a form with the programmer-definable interfaces, which will be used to enforce certain layouts across opaque contexts.

# runtime

There will be a special `runtime` object which is used to specify certain automatic behaviors. Through the `runtime` object, behind-the-scenes and implicit behavior can be fully defined. A block of code can be labeled `runtime (runtimeObj)`, which switches the runtime context it uses. When no particular `runtime` object is used, the program's behavior is fully default. Overriding `runtime` generally *inlines* the defined operations rather than actually using expensive indirection. `runtime` objects will be able to provide hooks for a variety of implicit and behind-the-scenes behaviors. Through it, scheduling, allocation, garbage collection, and multithreading can be defined. Another hook that can be intercepted by the `runtime` object is that an arbitrary operation can be performed on every value when it enters and exits scope. A powerful but highly obfuscated hook can be that memory accesses (reads and writes) can be interpreted by a runtime. This one should be used with extreme caution, but it can make things like safe multithreading and remote procedure calls well-defined, safe, and rewindable (for example, the runtime could emit a log of memory operations relative to a base pointer rather than actually carrying them out, and those logs could be consolidated and merged at various central points). All `runtime` overrides are risky territory, and they should quarantined as much as possible.

`runtime` exists as both an explanation for various implicit behaviors and a way to override them. The compiler-default implicit behaviors will generally be very reasonable, and overriding them should happen when specialization is necessary. A vast majority of codebases will never need to touch them; a good rule of thumb is that codebases which never hit runtime-related walls in C++ would never have overridden the runtime if they were written in this language.

The default runtime provides no garbage collector, no scheduler, an allocator which is transparent to the standard library, and no multithreader.

The default runtime does handle construction and destruction. Its behavior for when an object enters scope is to call `instance@instance_init` (this happens after any compound initializer applies). Its behavior for when an object leaves scope is to call `instance@instance_delete`, which is special. `delete` is a pseudomember, but it is undefined. A label call to it has no effect. It implicitly becomes defined when a `delete` label is placed in a struct's definition. The language reserves the right to define various special member names and canonicalize them as pseudomembers.

Compiling can transfer to a bytecode that is most of the complete text, but encodes inlining requirements so that objects can still be optimally linked against. Labels are defined by which members must be set, and which members may be returned. The callsite must verify these things before a call will succeed. No analysis about whether a label call is satisfiable will occur until a callsite attempts to satisfy it.

`runtime` will be a contract rather than a special type. Hooks will be labels, and any struct which satisfies a logical runtime configuration will be a valid runtime. An unspecified hook will simply be unimplemented. A runtime fails to be satisfying when an operation requires an absent hook. Most operations do not require any hooks at all, so a completely empty runtime will usually cause no problems except that the usual default behaviors will not trigger.

`pod` is a qualifier that prevents a declaration from being visible to the runtime on scope entry.

`manual` is the same qualifier for scope exit.

`param` implies both `pod` and `manual`.

Already-linked elf binaries generally must be treated as fully opaque; however, it may be possible for a C-targeted tool to analyze source code and generate the necessary context for this language. Those libraries only take `pod` structs and return a single type anyway, so it should be relatively inconsequential.

# `scheduler`s, `resolver`s, `blocking`, `resumable`, `dependency`, & general asynchronous patterns

// THESE ARE SWIFTLY-TYPED NOTES; they will be expanded and clarified later.

The language uses a complete, automatic asynchronous re-entrancy system through the above interfaces.

A `scheduler` is an interface that satisfies the language's requirements for orchestrating async.

A `resolver` is a property of a `scheduler`, and it is relied on by blocking leaf nodes which need to await some circumstance they do not control such as OS operations. The `scheduler` is responsible for calling on its `resolvers` in its main loop.

`resumable` is a base interface that lets a callee engage with the re-entrancy mechanisms this language uses.

# stacks

`stack` is a type that can hold instances of the current frame. A `stack` type variable, like a `label`, is instantiated specifically to the type of struct it's declared inside. `stack` variables are used to `goto` locally while cloning the current context and returning to it afterward. Local recursion is normally restricted to only fully tail-callable cases (and therefore becomes manual loop control flow), but `stack` type variables allow full local recursion. The compiler can treat `stack` frames lazily and analyze for whether it actually must store the complete struct for every recursion. There can only be one `stack` declaration per frame.

`stack`s can be provided specifiers that give the programmer finer control. These are the "tail specifier" and the "size specifier".

The tail specifier is in the form `stack(...members) myStack`, and it specifies which members need a unique slot on each frame. The remaining members are shared across the base and all additional frames. If a member is absent from the tail specifier, and you set it in one frame, the same set occurs in every frame because they all use the same storage.

As-is, it is impossible for a struct to return to a base calling context from a recursively-placed frame. The caller for a recursively-placed frame is necessarily either the base call (the one with zero recursion) or another recursively-placed frame. The size specifier enables a special return pattern that returns from a recursing context to the base caller. The `super` keyword modifies `yield` and `return` to do so, and it can only be called inside a context containing a `stack` with a size specifier. As long as there is a size specifier of any kind, `super` may modify returns and yields.

This is useful for early termination / unwinding when recursing, which is a good reason to have it, but the real reason is more powerful. After a `super return`, the whole stack that has been allocated up to this point will persist, with the inner `stack` variable being indexible. This allows unterminated structs with `stack`s inside to be arraylike.

The size specifier is in the form `stack[n]`, and it specifies a limit to how many frames can exist at once. This allows the compiler to place the whole region required by the struct and its `stack` onto the real stack.

The size specifier can take an empty `[]`, in which case the caller provides `n` via the declaration in the form `type[n] myStruct;`.

The two specifiers can be composed together in either order.

A struct containing a `stack` is indexable as an alias to that stack.

All structs have a `stack` pseudomember, which is the specific type-instance of `stack` used by that struct. `myStruct_stack` (not `myStruct.myStruct_stack`) is the same outside the struct as `stack` is inside the struct (it is a binding to the private `stack` type owned by `myStruct`). It can have tail and size specifiers applied. When used outside the struct, it is not considered "set" until it has been assigned a base `myStruct`.

`ref` is a type which is actually a pure reduction of `stack`; it cannot be indexed into or called against. Inside the struct, it can be used as a handle into a specific frame in the `stack`; outside, it is accessible as a pseudomember type `myStruct_ref`. A `myStruct_ref` can hold a pointer to a 

// TODO: rewrite to be better-suited to recent notions of allocators

`n` can be either a constant integer or an instance of the `allocator` class of structs. `allocator` structs get access to special operations that allow them to reserve stack and heap space, move data from allocation to allocation, defer to builtin stack behavior until some breakpoint, etc. They can query, for a stack allocation, whether a higher address is currently claimed and what that address is (allowing them to treat the stack as an unbounded bump allocator until they run into a conflict). The stack grows upward per this memory model so that indexing into `stack`-type data is straightforward.

An allocator struct should define special labels that will be called by the runtime automatically when (for example) the stack they are responsible for would overflow.

An allocator can also intercept the indexing operation using its own special labels. Behind the scenes, it is type-agnostic, and simply maps indices to base addresses for stack frames. It is allowed to return an `undef` type, which the wrapping context will forward to the index expression. This means the caller may be required to handle the return before using it. If it produces a value, that value must be in the member named `ret`; otherwise, the member named `err` will be set. If the caller wants to be as lazy as if the allocator always returned a regular value, it can ignore the potential for an error and:

```
    int x = (myStruct[10].ret).x;
```

This is the default unsafe route for if you use a custom index mapper but still want to just ignore buffer overflows.

A builtin allocator that can be provided to a `stack` type is the type `unbounded{bool manual, int initialSize, bool checkAccess}` (the init code handles defaults for these members if they are not set). This default builtin allows the compiler to use heap storage to accommodate any size. This is the only time heap memory will be managed by the compiler. It does not have to use heap storage, and it may manage its memory any way it likes as long as it preserves semantics. The compiler should really treat `unbounded` as a symbol enabling the efficient native management strategy rather than actually honoring it as a dynamic object.

constant integers, when passed to the size specifier, are really just a special notation for the builtin `bounded{int size, bool checkAccess}`. Then again, this is never fully true, as the compiler will treat an integer constant as an opportunity to use the native management strategy for bounded stacks. It's a lie on one end and a lie on the other, but it is still conceptually calling upon a builtin allocator.

An allocator struct's `destroy` label is called when the stack it is responsible for goes out of scope. You can manage your allocator so that its `destroy` is configurable to be a no-op, allowing you to manage it manually. Any stack memory owned by an allocator at destruction-time *is* freed no matter what, so its `destroy` label should make sure all data is on the heap in the manual case.

A `stack` whose tail specifies only one member is defined in-memory to be an aligned array of that member's type. This is important for the `bytecrawler` type which is explained further down.

`extend` and `retract` are active pseudomembers of each `stack` which take an integer argument `n`, push `n` uninitialized frames or remove `n` frames respectively, and re-enter within the top frame, which is initialized to be a copy of the base from which the member was invoked.

// TODO: integrate stacks into variadic negotiation

# misc

Given this set of definitions and a reasonably optimizing compiler, there is no need for other, non-struct functions. The whole program can be defined in terms of frames and the re-enterable control flow of operations to perform on them. In this model, there are no arguments; just the state of the trailing temp members and a potential enforcement that they were set visibly.

Generic callbacks can define a base struct type with no labels for callers to do as they please with. Structs can be extended after their definition using `struct extend myStruct {};`. `struct` can also be `frame`. This lets the callback definition site add params, temps, and even members to the struct definition as necessary. The canonical definition of that base type has all members, but the compiler can reasonably strip anything that never needs to be accessed or passed through an ABI. This is highly compatible with the concept of local notions; `label x for y` is treated as an isolated extension of a struct definition, so it can add arbitrary necessary members and context. This is free for the functions which do not include that context inside their notion and do not require fullness.

The `?` suffix operator (which has no ternary function) is used to query whether a `pod` variable has been assigned to. It may require to be `tagged` in order to know whether it has been assigned to.

`bytecrawler` Is the window into all the remaining wacky pointer business you could get up to. `bytecrawler` Is essentially `void*`. It takes a specifier notating the unit size to be used in arithmetic. A constant value must be provided. A type can be provided, and the crawler will be initialized to point to that type. It is dereferenced with a plain old `*`. When setting to it, it infers the type of the expression and obediently sets what it points to to a value of that type. If its current size does not guarantee it points to a valid alignment for that type, there can be a warning. The align operator takes a bytecrawler and a constant alignment (or type, becoming that type's alignment) and produces an aligned bytecrawler to the *next* aligned slot. An unconditional b = align(b, n) before a set can silence warnings about alignment. Because stack and ref accomodate the regular pointer use cases very safely, the bytecrawler is actually a lot freer than C++, for example. No strict types are necessary; it is just a handle into memory that can traverse as it pleases
