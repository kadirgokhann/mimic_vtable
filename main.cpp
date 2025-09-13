// vtable_mimic.cpp
#include <cstdio>
#include <cstdlib>
#include <cstring>

// --- "ABI": function signatures every slot will use ---
using DestroyFn = void(*)(void*);
using FooFn     = void(*)(void*);
using BarFn     = void(*)(void*);

// --- Our hand-rolled vtable layout ---
struct VTable {
    DestroyFn destroy;
    FooFn     foo;
    BarFn     bar;
};

// --- "Base object" layout: first field is the vptr (like real C++ ABI) ---
struct Obj {
    const VTable* vptr; // points to the class's vtable
    int payload;        // pretend member data
};

// --- Implementations for a 'Base' class ---
namespace BaseImpl {
    void destroy(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::puts("[Base::~Base] destroying");
        // since we used placement-new–style construction, only free memory here if we allocated.
        // in this demo, objects are on the stack; nothing to free.
        (void)o;
    }
    void foo(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::printf("[Base::foo] payload=%d\n", o->payload);
    }
    void bar(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::printf("[Base::bar] payload=%d\n", o->payload);
    }

    // singleton vtable for Base
    const VTable vtable = {
        &destroy,
        &foo,
        &bar
    };
}

// --- Implementations for a 'Derived' class overriding foo/bar ---
namespace DerivedImpl {
    void destroy(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::puts("[Derived::~Derived] destroying (then Base dtor could run)");
        (void)o;
    }
    void foo(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::printf("[Derived::foo]  payload=%d (overrides Base::foo)\n", o->payload);
    }
    void bar(void* self) {
        Obj* o = static_cast<Obj*>(self);
        std::printf("[Derived::bar] payload=%d (overrides Base::bar)\n", o->payload);
    }

    // singleton vtable for Derived
    const VTable vtable = {
        &destroy,
        &foo,
        &bar
    };
}

// --- "Constructors" that set the vptr, like C++ ctors do implicitly ---
void Base_ctor(Obj* o, int payload) {
    o->vptr   = &BaseImpl::vtable;
    o->payload = payload;
}
void Derived_ctor(Obj* o, int payload) {
    o->vptr   = &DerivedImpl::vtable;
    o->payload = payload;
}

// --- "Virtual calls": these are what the compiler would synthesize as indirect calls ---
void call_destroy(Obj* o) { o->vptr->destroy(o); }
void call_foo(Obj* o)     { o->vptr->foo(o); }
void call_bar(Obj* o)     { o->vptr->bar(o); }

int main() {
    std::puts("=== constructing objects ===");
    Obj a{}, b{};

    // a acts like Base; b acts like Derived
    Base_ctor(&a, 10);
    Derived_ctor(&b, 42);

    std::puts("\n=== printing vptrs (vtable addresses) ===");
    std::printf("a.vptr = %p (Base vtable)\n", (const void*)a.vptr);
    std::printf("b.vptr = %p (Derived vtable)\n", (const void*)b.vptr);

    std::puts("\n=== indirect calls via vtable ===");
    call_foo(&a); // uses BaseImpl::foo via a.vptr->foo
    call_bar(&a);
    call_foo(&b); // uses DerivedImpl::foo via b.vptr->foo
    call_bar(&b);

    std::puts("\n=== dynamic dispatch in action (polymorphic use) ===");
    Obj* arr[2] = { &a, &b };
    for (Obj* p : arr) {
        // same call site; behavior depends on which vtable p->vptr points to
        call_foo(p);
        call_bar(p);
    }

    std::puts("\n=== swapping vtables at runtime (simulating a cast/retarget) ===");
    // this is just to illustrate the mechanism; real C++ would not do this.
    a.vptr = &DerivedImpl::vtable; // 'a' now behaves like Derived
    call_foo(&a);
    call_bar(&a);

    std::puts("\n=== destructors via vtable ===");
    call_destroy(&a);
    call_destroy(&b);

    std::puts("\n(done)");
    return 0;
}
