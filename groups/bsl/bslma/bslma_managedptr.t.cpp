// bslma_managedptr.t.cpp                                             -*-C++-*-
#include <bslma_managedptr.h>

#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_testallocator.h>
#include <bslma_testallocatormonitor.h>
#include <bslmf_assert.h>
#include <bsls_asserttest.h>
#include <bsls_bsltestutil.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace BloombergLP;

//=============================================================================
//                             TEST PLAN
//                             ---------
// The 'bslma_managedptr' component provides a small number of classes that
// combine to provide a common solution to the problem of managing and
// transferring ownership of a dynamically allocated object.  It further
// contains a number of private classes to supply important implementation
// details, while the test driver introduces a reasonable amount of test
// machinery in order to carefully observe the correct handling of callbacks.
// We choose to test each class in turn, starting with the test machinery,
// according to their internal levelization in the component implementation.
//
// [ 2]   Test machinery
// [ 3]   imp. class bslma::ManagedPtr_Ref
// [4-5]  (tested classes migrated to their own components)
// [6-15] class bslma::ManagedPtr
// [16]   class bslma::ManagedPtrNilDeleter   [DEPRECATED]
// [17]   class bslma::ManagedPtrNoOpDeleter
//
// Further, there are a number of behaviors that explicitly should not compile
// by accident that we will provide tests for.  These tests should fail to
// compile if the appropriate macro is defined.  Each such test will use a
// unique macro for its feature test, and provide a commented-out definition
// of that macro immediately above the test, to easily enable compiling that
// test while in development.  Below is the list of all macros that control
// the availability of these tests:
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_SWAP_FOR_DIFFERENT_TYPES
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_ASSIGN_FROM_INCOMPATIBLE_TYPE
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_DEREFERENCE_VOID_PTR
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_CONVERT_TO_REF_FROM_CONST
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_MOVE_CONSTRUCT_FROM_CONST
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_INCOMPATIBLE_POINTERS
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_FACTORY
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_DELETER
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_LOAD_INCOMPATIBLE_TYPE
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_FACTORY
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_DELETER
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_CONSTRUCT_FROM_INCOMPATIBLE_POINTER
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_COMPARISON
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_ORDERING
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_COMPARISON
//  #define BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_ORDERING
//-----------------------------------------------------------------------------
//                         bslma::ManagedPtr
//                         -----------------
// We test 'bslma::ManagedPtr' incrementally, increasing the functionality that
// can be relied upon in later tests.  Starting with the default constructor,
// we will demonstrate that any valid state can be attained with the methods
// 'load' and 'loadAlias'.  In turn, this will be used to prove the correct
// behavior of the basic accessors, which are assumed correct but minimally
// relied up for the initial tests.  This allows us to test the remaining
// constructors, the move-semantic operations, and finally any other operations
// of the class.  Negative testing will ensure that all expected assertions are
// present and correct in the implementation, and a number of compile-fail
// tests will ensure that attempts to construct invalid managed pointers are
// caught early by the compiler, ideally with a helpful error diagnostic.
//-----------------------------------------------------------------------------
// [ 6] ManagedPtr();
// [ 6] ManagedPtr(bsl::nullptr_t);
// [ 6] template<class TARGET_TYPE> ManagedPtr(TARGET_TYPE *ptr);
// [11] ManagedPtr(ManagedPtr& original);
// [11] ManagedPtr(ManagedPtr_Ref<ELEMENT_TYPE> ref);
// [12] ManagedPtr(ManagedPtr<OTHER> &alias, TYPE *ptr)
// [10] ManagedPtr(TYPE *ptr, FACTORY *factory)
// [10] ManagedPtr(TYPE *ptr, void *factory,void(*deleter)(TYPE*, void*))
// [ 6] ~ManagedPtr();
// [11] operator ManagedPtr_Ref<OTHER>();
// [ 7] void load(nullptr_t=0,nullptr_t=0,nullptr_t=0);
// [ 7] template<class TARGET_TYPE> void load(TARGET_TYPE *ptr);
// [ 7] void load(TYPE *ptr, FACTORY *factory)
// [ 7] void load(TYPE *ptr, nullptr_t, void (*deleter)(TYPE *, void*));
// [ 7] void load(TYPE *ptr, void *factory, void (*deleter)(void *, void*));
// [ 7] void load(TYPE *ptr, void *factory, void (*deleter)(TYPE *, void*));
// [ 7] void load(TYPE *ptr, FACTORY *factory, void(*deleter)(TYPE *,FACTORY*))
// [ 8] void loadAlias(ManagedPtr<OTHER> &alias, TYPE *ptr)
// [13] void swap(ManagedPt& rhs);
// [14] ManagedPtr& operator=(ManagedPtr &rhs);
// [14] ManagedPtr& operator=(ManagedPtr_Ref<ELEMENT_TYPE> ref);
// [15] void clear();
// [15] bsl::pair<TYPE*,ManagedPtrDeleter> release();
// [  ] TARGET_TYPE *release(ManagedPtrDeleter *deleter);
// [ 9] operator BoolType() const;
// [ 9] TYPE& operator*() const;
// [ 9] TYPE *operator->() const;
// [ 9] TYPE *ptr() const;
// [ 9] const ManagedPtrDeleter& deleter() const;
//
// [16] class ManagedPtrNilDeleter
// [17] class ManagedPtrNoOpDeleter
//
// [ 3] imp. class ManagedPtr_Ref
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 2] TESTING TEST MACHINERY
// [18] CASTING EXAMPLE
// [19] USAGE EXAMPLE
// [-1] VERIFYING FAILURES TO COMPILE

//=============================================================================
//                    STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
int testStatus = 0;

namespace {

void aSsErT(bool b, const char *s, int i)
{
    if (b) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }

}

}  // close unnamed namespace

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define ASSERT       BSLS_BSLTESTUTIL_ASSERT
#define LOOP_ASSERT  BSLS_BSLTESTUTIL_LOOP_ASSERT
#define LOOP0_ASSERT BSLS_BSLTESTUTIL_LOOP0_ASSERT
#define LOOP1_ASSERT BSLS_BSLTESTUTIL_LOOP1_ASSERT
#define LOOP2_ASSERT BSLS_BSLTESTUTIL_LOOP2_ASSERT
#define LOOP3_ASSERT BSLS_BSLTESTUTIL_LOOP3_ASSERT
#define LOOP4_ASSERT BSLS_BSLTESTUTIL_LOOP4_ASSERT
#define LOOP5_ASSERT BSLS_BSLTESTUTIL_LOOP5_ASSERT
#define LOOP6_ASSERT BSLS_BSLTESTUTIL_LOOP6_ASSERT
#define ASSERTV      BSLS_BSLTESTUTIL_ASSERTV

#define Q   BSLS_BSLTESTUTIL_Q   // Quote identifier literally.
#define P   BSLS_BSLTESTUTIL_P   // Print identifier and value.
#define P_  BSLS_BSLTESTUTIL_P_  // P(X) without '\n'.
#define T_  BSLS_BSLTESTUTIL_T_  // Print a tab (w/o newline).
#define L_  BSLS_BSLTESTUTIL_L_  // current Line number

// ============================================================================
//                  NEGATIVE-TEST MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ASSERT_SAFE_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS(EXPR)
#define ASSERT_SAFE_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(EXPR)
#define ASSERT_PASS(EXPR)      BSLS_ASSERTTEST_ASSERT_PASS(EXPR)
#define ASSERT_FAIL(EXPR)      BSLS_ASSERTTEST_ASSERT_FAIL(EXPR)
#define ASSERT_OPT_PASS(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_PASS(EXPR)
#define ASSERT_OPT_FAIL(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_FAIL(EXPR)

#define ASSERT_SAFE_PASS_RAW(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS_RAW(EXPR)
#define ASSERT_SAFE_FAIL_RAW(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL_RAW(EXPR)
#define ASSERT_PASS_RAW(EXPR)      BSLS_ASSERTTEST_ASSERT_PASS_RAW(EXPR)
#define ASSERT_FAIL_RAW(EXPR)      BSLS_ASSERTTEST_ASSERT_FAIL_RAW(EXPR)
#define ASSERT_OPT_PASS_RAW(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_PASS_RAW(EXPR)
#define ASSERT_OPT_FAIL_RAW(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_FAIL_RAW(EXPR)

// ============================================================================
//                               TEST APPARATUS
// ----------------------------------------------------------------------------

//=============================================================================
//            MACROS TO TRANSPARENTLY TEST DIFFERENT (BETA) SEMANTICS
//-----------------------------------------------------------------------------
// Note that the macros defined in this section are entirely transitional, to
// support testing code that may or may not be adjusted to handle the new BDE
// smart pointer semantics.
//
// If the macro BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521
// is defined, then the original smenatics for constucting a mmanaged pointer
// with a single pointer argument are in force, which assume that the passed
// pointer was constructed dynamically using the default allocator.  By default
// we now assume the C++11 shared_ptr semantic, which is that a naked 'new'
// call was used.

#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
#   define BSLMA_IMPLICIT_ALLOCATOR (da)
#else
#   define BSLMA_IMPLICIT_ALLOCATOR
#endif
    // The 'BSLMA_IMPLICIT_ALLOCATOR' macro can be used in a 'new' expression
    // to get default semantic for dynamically allocating an object whose
    // address will be passed to a managed pointer constructor or 'load' method
    // without explicitly passing an allocator.  This assumes that the
    // default allocator is aliased as 'da'
//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------
namespace {

bool g_verbose;
bool g_veryVerbose;
bool g_veryVeryVeryVerbose;

class MyTestObject;
class MyDerivedObject;

typedef MyTestObject TObj;
typedef bslma::ManagedPtr<MyTestObject> Obj;
typedef bslma::ManagedPtr<const MyTestObject> CObj;
typedef MyDerivedObject TDObj;
typedef bslma::ManagedPtr<MyDerivedObject> DObj;
typedef bslma::ManagedPtr<void> VObj;

//=============================================================================
//                         HELPER CLASSES FOR TESTING
//-----------------------------------------------------------------------------
// The 'bsls_IsPolymorphic' trait does not work correctly on the following two
// platforms, which causes 'bslma::DeleterHelper' to dispatch to an
// implementation that cannot compile.
#if !defined(BSLS_PLATFORM_CMP_GNU) && !defined(BSLS_PLATFORM_CMP_HP)
#define BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE
#endif

struct Base {
    explicit Base(int *deleteCount)
    : d_count_p(deleteCount)
    {
    }

    ~Base() { ++*d_count_p; }

    int *d_count_p;
};

#if defined BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE
struct Base1 : virtual Base {
    explicit Base1(int *deleteCount = 0)
    : Base(deleteCount)
    , d_padding()
    {
    }

    ~Base1() { *d_count_p += 9; }

    char d_padding;
};

struct Base2 : virtual Base {
    explicit Base2(int *deleteCount = 0)
    : Base(deleteCount)
    , d_padding()
    {
    }

    ~Base2() { *d_count_p += 99; }

    char d_padding;
};

struct Composite : Base1, Base2 {
    explicit Composite(int *deleteCount)
    : Base(deleteCount)
    , d_padding()
    {
    }

    ~Composite() { *d_count_p += 891; }

    char d_padding;
};
#else
struct Base1 : Base {
    explicit Base1(int *deleteCount)
    : Base(deleteCount)
    , d_padding()
    {
    }

    ~Base1() { *d_count_p += 9; }

    char d_padding;
};

struct Base2 : Base {
    explicit Base2(int *deleteCount)
    : Base(deleteCount)
    , d_padding()
    {
    }

    ~Base2() { *d_count_p += 99; }

    char d_padding;
};

struct Composite : Base1, Base2 {
    explicit Composite(int *deleteCount)
    : Base1(deleteCount)
    , Base2(deleteCount)
    , d_count_p(deleteCount)
    {
    }

    ~Composite() { *d_count_p += 890; }

    int *d_count_p;
};
#endif

// The next three types are used for more general testing of types using
// multiple inheritance, and vtables.  They specifically support test case 11,
// and to not currently need to support the policy-driven generative testing
// framework.

class BaseInt1 {
  public:
    int d_data;  // public data as we want to inspect object layouts

    BaseInt1()
    : d_data(1)
    {
    }

    virtual int data1() const { return d_data; }
    virtual int data()  const { return d_data; }
};

class BaseInt2 {
  public:
    int d_data;  // public data as we want to inspect object layouts

    BaseInt2()
    : d_data(2)
    {
    }

    virtual int data()  const { return d_data; }
    virtual int data2() const { return d_data; }
};

class CompositeInt3 : public BaseInt1, public BaseInt2 {
  public:
    int d_data;  // public data as we want to inspect object layouts

    CompositeInt3()
    : d_data(3)
    {
    }

    virtual int data2() const { return d_data + d_data; }
    virtual int data1() const { return d_data * d_data; }
    virtual int data()  const { return d_data; }
};


// The next set of types are the primary test types used within the policy
// driven generative testing framework.

class MyTestObject {
    // This test-class serves three purposes.  It provides a base class for the
    // test classes in this test driver, so that derived -> base conversions
    // can be tested.  It also signals when its destructor is run by
    // incrementing an externally managed counter, supplied when each object
    // is created.  Finally, it exposes an internal data structure that can be
    // use to demonstrate the 'bslma::ManagedPtr' aliasing facility.

    // DATA
    volatile int *d_deleteCounter_p;
    mutable int   d_value[2];

  public:
    // CREATORS
    explicit MyTestObject(int *counter);

    // Use compiler-generated copy constructor and assignment operator
    // MyTestObject(MyTestObject const& orig);
    // MyTestObject operator=(MyTestObject const& orig);

    virtual ~MyTestObject();
        // Destroy this object.

    // ACCESSORS
    int *valuePtr(int index = 0) const;

    volatile int *deleteCounter() const;
};

MyTestObject::MyTestObject(int *counter)
: d_deleteCounter_p(counter)
, d_value()
{
}

MyTestObject::~MyTestObject()
{
    ++(*d_deleteCounter_p);
}

inline
int *MyTestObject::valuePtr(int index) const
{
    BSLS_ASSERT_SAFE(2 > index);

    return d_value + index;
}

volatile int* MyTestObject::deleteCounter() const
{
    return d_deleteCounter_p;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class MyDerivedObject : public MyTestObject
{
    // This test-class has the same destructor-counting behavior as
    // 'MyTestObject', but offers a derived class in order to test correct
    // behavior when handling derived->base conversions.

  public:
    // CREATORS
    explicit MyDerivedObject(int *counter);
    // Use compiler-generated copy

    ~MyDerivedObject();
        // Increment the stored reference to a counter by 100, then destroy
        // this object.
};

inline
MyDerivedObject::MyDerivedObject(int *counter)
: MyTestObject(counter)
{
}

inline
MyDerivedObject::~MyDerivedObject()
{
    (*deleteCounter()) += 99; // +1 from base -> 100
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class MySecondDerivedObject : public MyTestObject
{
    // This test-class has the same destructor-counting behavior as
    // 'MyTestObject', but offers a second, distinct, derived class in order to
    // test correct behavior when handling derived->base conversions.

  public:
    // CREATORS
    explicit MySecondDerivedObject(int *counter);
    // Use compiler-generated copy

    ~MySecondDerivedObject();
        // Increment the stored reference to a counter by 10000, then destroy
        // this object.
};

inline
MySecondDerivedObject::MySecondDerivedObject(int *counter)
: MyTestObject(counter)
{
}

inline
MySecondDerivedObject::~MySecondDerivedObject()
{
    (*deleteCounter()) += 9999;  // +1 from base -> 10000
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class CountedStackDeleter
{
    // DATA
    volatile int *d_deleteCounter_p;

  private:
    // NOT IMPLEMENTED
    CountedStackDeleter(const CountedStackDeleter&); //=delete;
    CountedStackDeleter& operator=(const CountedStackDeleter&); //=delete;

  public:
    // CREATORS
    explicit CountedStackDeleter(int *counter) : d_deleteCounter_p(counter) {}

    //! ~CountedStackDeleter();
        // Destroy this object.

    // ACCESSORS
    volatile int *deleteCounter() const { return d_deleteCounter_p; }

    void deleteObject(void *) const
    {
        ++*d_deleteCounter_p;
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct IncrementIntFactory
{
    void destroy(int *object)
    {
        ASSERT(object);
        ++*object;
    }
};

void incrementIntDeleter(int *ptr, IncrementIntFactory *factory)
{
    ASSERT(ptr);
    ASSERT(factory);

    factory->destroy(ptr);
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The two deleters defined below do not use the factory (or even object)
// argument to perform their bookkeeping.  They are typically used to test
// overloads taking 'NULL' factories.
int g_deleteCount = 0;

static void countedNilDelete(void *, void*)
{
    static int& deleteCount = g_deleteCount;
    ++g_deleteCount;
}

template<class TARGET_TYPE>
static void templateNilDelete(TARGET_TYPE *, void*)
{
    static int& deleteCount = g_deleteCount;
    ++g_deleteCount;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class ELEMENT_TYPE>
class AllocatorDeleter
{
    // This class provides a 'bslma::ManagedPtr' deleter that does *not* derive
    // from 'bslma::Allocator'.

  public:
    static void deleter(ELEMENT_TYPE *ptr, bslma::Allocator *alloc)
    {
        BSLS_ASSERT_SAFE(0 != ptr);
        BSLS_ASSERT_SAFE(0 != alloc);

        alloc->deleteObject(ptr);
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct OverloadTest {
    // This struct provides a small overload set taking managed pointer values
    // with similar looking (potentially related) types, to be sure there are
    // no unexpected ambiguities or conversions.

    static int invoke(bslma::ManagedPtr<void>)     { return 0; }
    static int invoke(bslma::ManagedPtr<int>)      { return 1; }
    static int invoke(bslma::ManagedPtr<const int>){ return 2; }
        // Return an integer code reporting which specific overload was called.
};

//=============================================================================
//                              CREATORS TEST
//=============================================================================

namespace CREATORS_TEST_NAMESPACE {

struct SS {
    // DATA
    char  d_buf[100];
    int  *d_numDeletes_p;

    // CREATORS
    explicit SS(int *numDeletes)
    {
        d_numDeletes_p = numDeletes;
    }

    ~SS()
    {
        ++*d_numDeletes_p;
    }
};

typedef bslma::ManagedPtr<SS> SSObj;
typedef bslma::ManagedPtr<char> ChObj;

}  // close namespace CREATORS_TEST_NAMESPACE

}  // close unnamed namespace

//=============================================================================
//                    FILE-STATIC FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

static void myTestDeleter(TObj *object, bslma::TestAllocator *allocator)
{
    allocator->deleteObject(object);
    if (g_verbose) {
        printf("myTestDeleter called\n");
    }
}

static bslma::ManagedPtr<MyTestObject>
returnManagedPtr(int *numDels, bslma::TestAllocator *allocator)
{
    MyTestObject *p = new (*allocator) MyTestObject(numDels);
    bslma::ManagedPtr<MyTestObject> ret(p, allocator);
    return ret;
}

static bslma::ManagedPtr<MyDerivedObject>
returnDerivedPtr(int *numDels, bslma::TestAllocator *allocator)
{
    MyDerivedObject *p = new (*allocator) MyDerivedObject(numDels);
    bslma::ManagedPtr<MyDerivedObject> ret(p, allocator);
    return ret;
}

static bslma::ManagedPtr<MySecondDerivedObject>
returnSecondDerivedPtr(int *numDels, bslma::TestAllocator *allocator)
{
    MySecondDerivedObject *p = new (*allocator) MySecondDerivedObject(numDels);
    bslma::ManagedPtr<MySecondDerivedObject> ret(p, allocator);
    return ret;
}

static void doNothingDeleter(void *object, void *)
{
    ASSERT(object);
}

template <class T>
void validateManagedState(unsigned int                   LINE,
                          const bslma::ManagedPtr<T>&     obj,
                          const void                    *ptr,
                          const bslma::ManagedPtrDeleter& del)
{
    // Testing the following properties of the specified 'obj'
    //   operator BoolType() const;
    //   TYPE& operator*() const;
    //   TYPE *operator->() const;
    //   TYPE *ptr() const;
    //   const bslma::ManagedPtrDeleter& deleter() const;

    bslma::TestAllocatorMonitor gam(dynamic_cast<bslma::TestAllocator*>
                                          (bslma::Default::globalAllocator()));
    bslma::TestAllocatorMonitor dam(dynamic_cast<bslma::TestAllocator*>
                                         (bslma::Default::defaultAllocator()));

    if (!ptr) {
        // Different negative testing constraints when 'ptr' is null.
        LOOP_ASSERT(LINE, false == obj);
        LOOP_ASSERT(LINE, !obj);
        LOOP_ASSERT(LINE, 0 == obj.operator->());
        LOOP_ASSERT(LINE, 0 == obj.ptr());

#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing\n");

        {
            bsls::AssertTestHandlerGuard guard;

            ASSERT_SAFE_FAIL(*obj);
            ASSERT_SAFE_FAIL(obj.deleter());
        }
#endif
    }
    else {
        // Different negative testing constraints when 'ptr' is null.
        ASSERT(true  == (bool)obj);
        ASSERT(false == !obj);

        T *arrow = obj.operator->();
        LOOP3_ASSERT(LINE, ptr, arrow, ptr == arrow);

        T * objPtr = obj.ptr();
        LOOP3_ASSERT(LINE, ptr, objPtr, ptr == objPtr);

        T &target = *obj;
        LOOP3_ASSERT(LINE, &target, ptr, &target == ptr);

        const bslma::ManagedPtrDeleter& objDel = obj.deleter();
        LOOP3_ASSERT(LINE, del, objDel, del == objDel);
    }

    ASSERT(gam.isInUseSame());
    ASSERT(gam.isMaxSame());

    ASSERT(dam.isInUseSame());
    ASSERT(dam.isMaxSame());
}

void validateManagedState(unsigned int                   LINE,
                          const bslma::ManagedPtr<void>&  obj,
                          void                          *ptr,
                          const bslma::ManagedPtrDeleter& del)
{
    // Testing the following properties of the specified 'obj'
    //   operator BoolType() const;
    //   void operator*() const;
    //   void *operator->() const;
    //   void *ptr() const;
    //   const bslma::ManagedPtrDeleter& deleter() const;

    bslma::TestAllocatorMonitor gam(dynamic_cast<bslma::TestAllocator*>
                                          (bslma::Default::globalAllocator()));
    bslma::TestAllocatorMonitor dam(dynamic_cast<bslma::TestAllocator*>
                                         (bslma::Default::defaultAllocator()));

    if (!ptr) {
        // Different negative testing constraints when 'ptr' is null.
        LOOP_ASSERT(LINE, false == obj);
        LOOP_ASSERT(LINE, !obj);
        LOOP_ASSERT(LINE, 0 == obj.operator->());
        LOOP_ASSERT(LINE, 0 == obj.ptr());
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing\n");

        {
            bsls::AssertTestHandlerGuard guard;

            ASSERT_SAFE_FAIL(obj.deleter());
        }
#endif
    }
    else {
        // Different negative testing constraints when 'ptr' is null.
        ASSERT(true  == (bool)obj);
        ASSERT(false == !obj);

        void *arrow = obj.operator->();
        LOOP3_ASSERT(LINE, ptr, arrow, ptr == arrow);

        void * objPtr = obj.ptr();
        LOOP3_ASSERT(LINE, ptr, objPtr, ptr == objPtr);

        const bslma::ManagedPtrDeleter& objDel = obj.deleter();
        LOOP3_ASSERT(LINE, del, objDel, del == objDel);

#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_DEREFERENCE_VOID_PTR)
        *obj;
#endif
    }

    ASSERT(gam.isInUseSame());
    ASSERT(gam.isMaxSame());

    ASSERT(dam.isInUseSame());
    ASSERT(dam.isMaxSame());
}

void validateManagedState(unsigned int                        LINE,
                          const bslma::ManagedPtr<const void>& obj,
                          const void                         *ptr,
                          const bslma::ManagedPtrDeleter&      del)
{
    // Testing the following properties of the specified 'obj'
    //   operator BoolType() const;
    //   void operator*() const;
    //   const void *operator->() const;
    //   const void *ptr() const;
    //   const bslma::ManagedPtrDeleter& deleter() const;

    bslma::TestAllocatorMonitor gam(dynamic_cast<bslma::TestAllocator*>
                                          (bslma::Default::globalAllocator()));
    bslma::TestAllocatorMonitor dam(dynamic_cast<bslma::TestAllocator*>
                                         (bslma::Default::defaultAllocator()));

    if (!ptr) {
        // Different negative testing constraints when 'ptr' is null.
        LOOP_ASSERT(LINE, false == obj);
        LOOP_ASSERT(LINE, !obj);
        LOOP_ASSERT(LINE, 0 == obj.operator->());
        LOOP_ASSERT(LINE, 0 == obj.ptr());
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing\n");

        {
            bsls::AssertTestHandlerGuard guard;

            ASSERT_SAFE_FAIL(obj.deleter());
        }
#endif
    }
    else {
        // Different negative testing constraints when 'ptr' is null.
        ASSERT(true  == (bool)obj);
        ASSERT(false == !obj);

        const void *arrow = obj.operator->();
        LOOP3_ASSERT(LINE, ptr, arrow, ptr == arrow);

        const void * objPtr = obj.ptr();
        LOOP3_ASSERT(LINE, ptr, objPtr, ptr == objPtr);

        const bslma::ManagedPtrDeleter& objDel = obj.deleter();
        LOOP3_ASSERT(LINE, del, objDel, del == objDel);

#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_DEREFERENCE_VOID_PTR)
        *obj;
#endif
    }

    ASSERT(gam.isInUseSame());
    ASSERT(gam.isMaxSame());

    ASSERT(dam.isInUseSame());
    ASSERT(dam.isMaxSame());
}


// 'debugprint' support for 'bsl' types

namespace BloombergLP {
namespace bslma {

void debugprint(const ManagedPtrDeleter& obj)
{
    printf("ManagedPtrDeleter[");
    printf("object: ");     bsls::debugprint(obj.object());
    printf(", factory: ");  bsls::debugprint(obj.factory());
    printf(", deleter: ");  bsls::debugprint(obj.deleter());
    printf("]");
}

}  // close namespace bslma
}  // close namespace BloombergLP

//=============================================================================
//                      'load' and constructor TESTING SUPPORT
//-----------------------------------------------------------------------------
// The following functions load a 'bslma::ManagedPtr' into a defined final
// state assuming that it is passed in with an initial state known to the
// calling function.  None of the following functions have their own test case,
// as they are vital implementation details of testing the constructors, and of
// testing the 'load' functions which, in turn, are later used to test the
// basic accessors.  However, these functions are very thoroughly exercised in
// the various 'load' and constructor test cases, in particular by taking an
// empty 'bslma::ManagedPtr' and taking it to the known state expected of each
// of these functions.  Similarly, we will test each transition from every
// possible initial state through each of these functions to validate all
// 'load' state transitions.  Essentially, these are implementation details of
// the 'load' test cases that may be deemed validated by that test case, and so
// safely relied on for all later test cases.
//
// Note that we are generating a very large test-space of test cases to ensure
// that all valid type-deductions and conversions are syntax-checked through
// the various overload sets for 'load' and 'bslma::ManagedPtr' constructors.
// Attempts to manually test the valid combinations demonstrated that the only
// way to be truly confident with this complex set of constraints was to
// exhaustively generate every testable combination.  These test tables are
// generated in a way to support automated tested of functions with varying
// signatures though a single, simple framework.
//
// Each function below has the same signature so that they can be used to
// populate a test table supporting table-driven testing techniques.  This will
// enable exhaustive testing of the state space and transitions of holding
// various kinds of 'bslma::ManagedPtr' objects.
//
// Each function performs the same set of operations in turn:
//: 1 Copy the initial values stored in passed pointers to compute the side-
//:   effects expected of calling 'load', typically observed as a consequence
//:   of destroying any held managed object.
//:
//: 2 'load' the specified pointer 'o' into the new defined state.
//:
//: 3 Set the new value for 'deleteDelta' for when this new state of 'o' is
//:   destroyed.
//:
//: 4 confirm the act of 'load'ing ran the expected deleters by comparing
//:   new state of 'deleteCount' with the computed value in (1).
//:
//: 5 confirm that each (defined) attribute of 'o' is in the expected state.
//
// The state combinations that need testing are invoking load with 0, 1, 2 or 3
// arguments.  Each combination should be tested for 'bslma::ManagedPtr'
// parameterized with
//: o 'MyTestObject'
//: o 'const MyTestObject'
//: o 'MyDerivedObject'
//: o 'void'
//: o 'const void'
//
// Additionally, there are a small number of corner cases where the base class
// does not have a virtual destructor, especially when the most derived object
// is using multiple inheritance.  These additional states are tested with
// smaller test tables for the 'Base', 'Base1', 'Base2' and 'Composite'
// hierarchy.  A basic set of these tests are also added to the test table for
// 'bslma::ManagedPtr<const void>'.
//
// The first pointer argument should be tested with a pointer of each of the
// following types:
//: o '0' literal
//: o 'MyTestObject *'
//: o 'const MyTestObject *'
//: o 'MyDerivedObject *'
//: o 'void *'
//: o 'const void *'
//
// When no 'factory' argument is passed, each function should behave as if the
// default allocator were passing in that place.
//
// The second factory argument should be tested with:
//: o '0' literal
//: o 'bslma::Allocator *'
//: o 'ta' to test the specific 'bslma::TestAllocator' derived factory type
//: 0 SOME OTHER FACTORY TYPE NOT DERIVED FROM BSLMA_TESTALLOCATOR [NOT TESTED]
//
// The 'deleter' argument will be tested with each of:
//: o '0' literal
//
// The 'line' and 'index' parameters describe the source line number at the
// call site, and any index into a function table to identify the specific
// invocation of this test function.  The 'useDefault' argument must be set to
// 'true' if the function allocates memory from the default allocator.  This is
// then used by the calling harness to know if it can check the default
// allocator's memory usage.
//
// The following chart describes the complete set of test scenarios, labeled
// with their corresponding function:
//:        Object  Code     Value
//:        ------  ----     -----
//:             -  (none)   use default (if any)
//:             0  Onull    null pointer literal
//:          base  Obase    pointer to allocated MyTestObject
//:    const base  OCbase   pointer to allocated 'const MyTestObject'
//:       derived  Oderiv   pointer to allocated 'MyDerivedObject'
//: const derived  OCderiv  pointer to allocated 'const MyDerivedObject'
//:
//:       Factory  Code     Value
//:       -------  ----     -----
//:             -  (none)   use default (if any)
//:             0  Fnull    null pointer literal
//:         bslma  Fbsl     'bslma::TestAllocator' cast to 'bslma::Allocator *'
//:          Test  Ftst     'bslma::TestAllocator' factory
//:       default  Fdflt    default allocator, passed as 'bslma::Allocator *'
//:
//: [No const factory support by default, but can 'deleter' support this?]
//: [Probably only through the deprecated interface, and no code can do this
//: yet?]
//:
//: Scenarios to consider:
//: "V(V* V*)" can hide many casting opportunities inside the function body.
//: This implies we may have many scenarios to test inside this one case, or
//: we may want to pick the most representative single case.  In fact, two
//: cases dominate our analysis, "V(bslma::Allocator, Base)", and "V(actual,
//: actual)".  The former can be explicitly coded up as a non-template
//: function.  The latter is already implemented as
//: 'bslma::ManagedPtr_FactoryDeleter'.  Note that there is a third category of
//: deleter, where the deleter function acts only on the 'object' parameter and
//: ignores the 'factory'.  This is an important case, as we must support '0'
//: literals and null pointers for factories based on existing code.  We can
//: test this case with a deleter that assumes the object was allocated using
//: the default allocator.  This gives us our 3 test scenarios for "V(V*,V*)".
//: The "V(V*,B*)" and "V(V*,D*)" cases could be tricky if the first "V*"
//: parameter is thought to be a type-erased factory that is cast back
//: internally.  We believe there are such cases in existing code, so must be
//: supported - we cannot assume the initial "V*" factory argument is ignored
//: by the deleter.  Here we will test just two forms, 'D' ignoring the factory
//: argument and using the default allocator to destroy the 'object', and 'B'
//: which destroys the 'object' by casting the 'factory' to
//: 'bslma::Allocator*'.
//:
//:      Deleter  Code   Value
//:      -------  ----   -----
//:            -  (none) use default (if any)
//:            0  Dnull  [ALL SUCH OVERLOADS ARE COMPILE-FAIL TEST CASES]
//:    V(V*, V*)  Dzero  a pointer variable with value '0'.
//:    V(V*, V*)  DvvF
//:    V(V*, V*)  DvvT
//:    V(V*, V*)  DvvD
//:    V(V*, B*)  DvbD
//:    V(V*, B*)  DvbB
//:    V(V*, D*)  DvdD
//:    V(V*, D*)  DvdB
//:    V(B*, V*)  Dbv
//:    V(B*, B*)  Dbb
//:    V(B*, D*)  Dbd
//:    V(T*, V*)  Dtv
//:    V(T*, B*)  Dtb
//:    V(T*, D*)  Dtd
//:
//: Deleter codes used above:
//:     V(X* Y*) is a function type, returning 'void' taking arguments of type
//:              'X*' and 'Y*'.
//:
//: Possible values of X:
//: o V void
//: o B bslma::Allocator
//: o T bslma::TestAllocator
//:
//: Possible values of Y:
//: o V void
//: o B MyTestClass
//: o D MyDerivedClass

//X doLoad

//X doLoadOnull
//X doLoadObase
//X doLoadOCbase
//X doLoadOderiv
//X doLoadOCderiv

//- doLoadOnullFbsl    [COMPILE-FAIL], but might permit
//- doLoadOnullFtst    [COMPILE-FAIL], but might permit
//X doLoadObaseFbsl
//X doLoadObaseFtst
//X doLoadOCbaseFbsl
//X doLoadOCbaseFtst
//X doLoadOderivFbsl
//X doLoadOderivFtst
//X doLoadOCderivFbsl
//X doLoadOCderivFtst

//X doLoadObaseFbslDzero
//X doLoadObaseFtstDzero
//X doLoadOderivFbslDzero
//X doLoadOderivFtstDzero

// TBD Can we store a 'void *' object with a knowledgeable factory?
//     (I think we require a knowledgeable deleter to handle such type erasure)

// WOULD THESE SERVE ANY PURPOSE, OR PURELY DELETER TESTS?
// Patterns that would form compile-fails
//   O*Fnull
//   O*Fdflt
//   O*FVtest
//   O*FVdflt
//   O*F*Dnull

// These stems match the above pattern, but are interesting when testing
// deleters

//: doLoadOnullFdflt
//: doLoadObaseFdflt
//: doLoadOCbaseFdflt
//: doLoadOderivFdflt
//: doLoadOCderivFdflt

//: doLoadOnullFnull
//: doLoadObaseFnull
//: doLoadOCbaseFnull
//: doLoadOderivFnull
//: doLoadOCderivFnull

namespace {

template <class POINTER_TYPE>
struct TestLoadArgs {
    // This struct holds the set of arguments that will be passed into a
    // policy based test function.  It collects all information for the range
    // of tests and expectations to be set up on entry, and reported on exit.

    int d_deleteCount;          // Delete counter, whose address will be passed
                                // to test object constructors.
    int d_deleteDelta;          // Expected change in delete counter when a new
                                // value is 'load'ed or destructor run.
    bool d_useDefault;          // Set to true if the test uses the default
                                // allocator.
    bslma::TestAllocator *d_ta; // pointer to a test allocator whose lifetime
                                // will outlast the function call.
    unsigned int d_config;      // Valid values are 0-3.
                                // The low-bit represents whether to pass a
                                // null for 'object', the second bit whether to
                                // pass a null for 'factory'
    bslma::ManagedPtr<POINTER_TYPE> *d_p; // pointer to the long-lived managed
                                         // pointer on which to execute tests
};

template <class POINTER_TYPE>
void validateTestLoadArgs(int callLine,
                          int testLine,
                          const TestLoadArgs<POINTER_TYPE> *args)
{
    // Assert pre-conditions that are appropriate for every call using 'args'.
    LOOP3_ASSERT(callLine, testLine, args->d_deleteCount,
                                                     0 == args->d_deleteCount);
    LOOP3_ASSERT(callLine, testLine, args->d_p,      0 != args->d_p);
    LOOP3_ASSERT(callLine, testLine, args->d_ta,     0 != args->d_ta);
}

//=============================================================================
//                          Target Object policies
// A Target Object policy consist of two members:
//: 1 A typedef, 'ObjectType', that reports the type of object to create
//: 2 An enum value 'DELETE_DELTA' reporting the expected change in
//:   'deleteCount' when the created object is destroyed.
//
// Note that the dynamic type of the object used in the test might be quite
// different to the static type of the created object described by this policy,
// notably for tests of 'bslma::ManagedPtr<void>'.
//
// List of available policies:
struct Obase;
struct OCbase;
struct Oderiv;
struct OCderiv;

// Policy implementations
struct Obase {
    typedef MyTestObject ObjectType;

    enum { DELETE_DELTA = 1 };
};

struct OCbase {
    typedef const MyTestObject ObjectType;

    enum { DELETE_DELTA = 1 };
};

struct Oderiv {
    typedef MyDerivedObject ObjectType;

    enum { DELETE_DELTA = 100 };
};

struct OCderiv {
    typedef const MyDerivedObject ObjectType;

    enum { DELETE_DELTA = 100 };
};

struct Ob1 {
    typedef Base1 ObjectType;

    enum { DELETE_DELTA = 10 };
};

struct Ob2 {
    typedef Base2 ObjectType;

    enum { DELETE_DELTA = 100 };
};

struct Ocomp {
    typedef Composite ObjectType;

    enum { DELETE_DELTA = 1000 };
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                             Factory Policies
// List of available policies:
struct Fbsl;
struct Ftst;
struct Fdflt;

// Policy implementations
struct Fbsl {
    typedef bslma::Allocator FactoryType;

    static FactoryType *factory(bslma::TestAllocator *f)
    {
        return f;
    }

    enum { USE_DEFAULT = false };
    enum { DELETER_USES_FACTORY = true};
};

struct Ftst {
    typedef bslma::TestAllocator FactoryType;

    static FactoryType *factory(bslma::TestAllocator *f)
    {
        return f;
    }

    enum { USE_DEFAULT = false };
    enum { DELETER_USES_FACTORY = true};
};

struct Fdflt {
    typedef bslma::Allocator FactoryType;

    static FactoryType *factory(bslma::TestAllocator *)
    {
        return bslma::Default::defaultAllocator();
    }

    enum { USE_DEFAULT = true };
    enum { DELETER_USES_FACTORY = false};
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                             Deleter Policies
// List of available policies:
template<class ObjectPolicy, class FactoryPolicy> struct DObjFac;
template<class ObjectPolicy, class FactoryPolicy> struct DObjVoid;
template<class ObjectPolicy, class FactoryPolicy> struct DVoidFac;
template<class ObjectPolicy, class FactoryPolicy> struct DVoidVoid;

// Policy implementations
template<class ObjectPolicy, class FactoryPolicy>
struct DObjFac {
    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    typedef void DeleterType(ObjectType*, FactoryType *);

    enum {DELETER_USES_FACTORY = FactoryPolicy::DELETER_USES_FACTORY};

    static void doDelete(ObjectType * object, FactoryType * factory)
    {
        if (DELETER_USES_FACTORY) {
            factory->deleteObject(object);
        }
        else {
            // Use default allocator as the deleter,
            // ignore the passed factory pointer
            bslma::Allocator *pDa = bslma::Default::defaultAllocator();
            pDa->deleteObject(object);
        }
    }

    static DeleterType *deleter()
    {
        return &doDelete;
    }
};

template<class ObjectPolicy, class FactoryPolicy>
struct DObjVoid {
    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    typedef void DeleterType(ObjectType*, void *);

    enum {DELETER_USES_FACTORY = FactoryPolicy::DELETER_USES_FACTORY};

    static void doDelete(ObjectType * object, void * factory)
    {
        if (DELETER_USES_FACTORY) {
            FactoryType *fac = reinterpret_cast<FactoryType *>(factory);
            fac->deleteObject(object);
        }
        else {
            // Use default allocator as the deleter,
            // ignore the passed factory pointer
            bslma::Allocator *pDa = bslma::Default::defaultAllocator();
            pDa->deleteObject(object);
        }
    }

    static DeleterType *deleter()
    {
        return &doDelete;
    }
};

// The 'ToVoid' metafunction supports tests that need to use a 'void' pointer
// representing a pointer to the test object, while also retaining the correct
// cv-qualification.
template <class TYPE>
struct ToVoid {
    typedef void type;
};

template <class TYPE>
struct ToVoid<const TYPE> {
    typedef const void type;
};

template <class TYPE>
struct ToVoid<volatile TYPE> {
    typedef volatile void type;
};

template <class TYPE>
struct ToVoid<const volatile TYPE> {
    typedef const volatile void type;
};

template<class ObjectPolicy, class FactoryPolicy>
struct DVoidFac {
    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    typedef typename ToVoid<ObjectType>::type VoidType;

    typedef void DeleterType(VoidType *, FactoryType *);

    enum {DELETER_USES_FACTORY = FactoryPolicy::DELETER_USES_FACTORY};

    static void doDelete(VoidType * object, FactoryType * factory)
    {
        ObjectType *obj = reinterpret_cast<ObjectType *>(object);
        if (DELETER_USES_FACTORY) {
            factory->deleteObject(obj);
        }
        else {
            // Use default allocator as the deleter,
            // ignore the passed factory pointer
            bslma::Allocator *pDa = bslma::Default::defaultAllocator();
            pDa->deleteObject(obj);
        }
    }

    static DeleterType *deleter()
    {
        return &doDelete;
    }
};

template<class ObjectPolicy, class FactoryPolicy>
struct DVoidVoid {
    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    typedef void DeleterType(void*, void *);

    enum {DELETER_USES_FACTORY = FactoryPolicy::DELETER_USES_FACTORY};

    static void doDelete(void * object, void * factory)
    {
        ObjectType *obj = reinterpret_cast<ObjectType *>(object);
        if (DELETER_USES_FACTORY) {
            FactoryType *fac = reinterpret_cast<FactoryType *>(factory);
            fac->deleteObject(obj);
        }
        else {
            // Use default allocator as the deleter,
            // ignore the passed factory pointer
            bslma::Allocator *pDa = bslma::Default::defaultAllocator();
            pDa->deleteObject(obj);
        }
    }

    static DeleterType *deleter()
    {
        return &doDelete;
    }
};

//=============================================================================
//                       POLICY BASED TEST FUNCTIONS
//=============================================================================
// The following set of functions use the policies defined in the previous
// section to construct a set of tests that will exhaustively cover the
// potential type-space of valid combinations of type for each set of arguments
// to 'bslma::ManagedPtr::load'.  Each is a function template, taking at least
// a single type parameter describing the type 'bslma::ManagedPtr' should be
// instantiated for.  Most function templates will take additional type
// arguments describing different policies that are used to define the
// functionality of that test.
// This decomposition into 11 test policies and 10 test functions allows us to
// generate over 200 distint test functions, that in turn may be specified for
// each of the 5 types we instantiate 'bslma::ManagedPtr' with for testings.
// Note that not all 200 tests are valid for each of the 5 types, and indeed
// many will not compile if instantiated.
// In order to sequentially test each state and permutation of state changes we
// generate large test tables for each of our 5 test types taking the address
// of each valid test function that can be instantiated.  For completeness and
// ease of auditing, we list all combinations of function and policy for each
// of the 5 test types, and comment out only those we believe must be disabled.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct TestCtorArgs {
    // This struct holds the set of arguments that will be passed into a
    // policy based test function.  It collects all information for the range
    // of tests and expectations to be set up on entry, and reported on exit.

    bool d_useDefault;  // Set to true if the test uses the default allocator
    unsigned int d_config; // Valid values are 0-3.  The low-bit represents
                           // whether to pass a null for 'object', the second
                           // bit whether to pass a null for 'factory'.
};


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// No policies needed for 'load' of empty managed pointers

template <class POINTER_TYPE>
void doConstruct(int callLine, int testLine, int index,
            TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 1 > args->d_config);

    bslma::ManagedPtr<POINTER_TYPE> testObject;

    POINTER_TYPE *ptr = testObject.ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);
}

template <class POINTER_TYPE>
void doConstructOnull(int callLine, int testLine, int index,
                 TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 1 > args->d_config);

    bslma::ManagedPtr<POINTER_TYPE> testObject(0);

    POINTER_TYPE *ptr = testObject.ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);
}

template <class POINTER_TYPE>
void doConstructOnullFnull(int callLine, int testLine, int index,
                      TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 1 > args->d_config);

    bslma::ManagedPtr<POINTER_TYPE> testObject(0, 0);

    POINTER_TYPE *ptr = testObject.ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);
}

template <class POINTER_TYPE>
void doConstructOnullFnullDnull(int callLine, int testLine, int index,
                           TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 1 > args->d_config);

    bslma::ManagedPtr<POINTER_TYPE> testObject(0, 0, 0);

    POINTER_TYPE *ptr = testObject.ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// A simple object policy governs loading a single argument
struct TestUtil {
    template <class TARGET_TYPE>
    static void *stripPointerType(TARGET_TYPE *ptr)
    {
        return const_cast<void*>(static_cast<const void*>(ptr));
    }
};

template<class POINTER_TYPE, class ObjectPolicy>
void doConstructObject(int callLine, int testLine, int index,
                       TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 2 > args->d_config);

    typedef typename ObjectPolicy::ObjectType ObjectType;

    const bool nullObject  = args->d_config & 1;

    const int expectedCount = nullObject
                            ? 0
                            : ObjectPolicy::DELETE_DELTA;
    int deleteCount = 0;
    ObjectType *pO = 0;
    if (nullObject) {
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO);

        const bslma::ManagedPtrDeleter del;

        validateManagedState(L_, testObject, 0, del);
    }
    else {
#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
        bslma::Allocator& da = *bslma::Default::defaultAllocator();
        pO = new(da) ObjectType(&deleteCount);
        args->d_useDefault = true;

        bslma::ManagedPtr<POINTER_TYPE> testObject(pO);

        typedef bslma::ManagedPtr_FactoryDeleter<ObjectType,bslma::Allocator>
                                                                  DeleterClass;
        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                          &da,
                                          &DeleterClass::deleter);

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
#else
        pO = new ObjectType(&deleteCount);
        args->d_useDefault = false;

        bslma::ManagedPtr<POINTER_TYPE> testObject(pO);

        typedef bslma::ManagedPtr_DefaultDeleter<ObjectType> DeleterClass;
        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                           0,
                                           &DeleterClass::deleter);

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
#endif
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, deleteCount,
                 expectedCount == deleteCount);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following functions load a 'bslma::ManagedPtr' object using a factory.
// We now require separate policies for Object and Factory types

template<class POINTER_TYPE, class FactoryPolicy>
void doConstructOnullFactory(int callLine, int testLine, int index,
                             TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 1 > args->d_config);

    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
    FactoryType *pAlloc = FactoryPolicy::factory(&ta);

    const bslma::ManagedPtrDeleter del;

    bslma::ManagedPtr<POINTER_TYPE> testObject(0, pAlloc);
    validateManagedState(L_, testObject, 0, del);
}

template<class POINTER_TYPE, class ObjectPolicy, class FactoryPolicy>
void doConstructObjectFactory(int callLine, int testLine, int,
                              TestCtorArgs *args)
{
    BSLMF_ASSERT(FactoryPolicy::DELETER_USES_FACTORY);

    LOOP3_ASSERT(callLine, testLine, args->d_config, 4 > args->d_config);

    const bool nullObject  = args->d_config & 1;
    const bool nullFactory = args->d_config & 2;

    // given a two-argument call to 'load', there is a problem only if
    // 'factory' is null while 'object' has a non-null value, as there is no
    // way to destroy the target object.  Pass a null deleter if that is the
    // goal.
    bool negativeTesting = !nullObject && nullFactory;

    // If we are negative-testing, we will create and destroy any target
    // object entirely within this function, so must track with a local counter
    // instead of the 'args' counter.

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);

    FactoryType *pAlloc = FactoryPolicy::factory(&ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    // Load the 'bslma::ManagedPtr' and check that the previous state is
    // correctly cleared.
    if (!negativeTesting) {
        typedef typename
        bslma::ManagedPtr_FactoryDeleterType<ObjectType,FactoryType>::Type
                                                                  DeleterClass;

        const bslma::ManagedPtrDeleter del;

        ObjectType  *pO = 0;
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF);

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
    }
    else {
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing null factory pointer\n");

        int deleteCount = 0;
        ObjectType  *pO = nullObject
                        ? 0
                        : new(*pAlloc)ObjectType(&deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }

        bsls::AssertTestHandlerGuard guard;

        ASSERT_SAFE_FAIL_RAW(
                           bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF));

        pAlloc->deleteObject(pO);

        LOOP_ASSERT(deleteCount, ObjectPolicy::DELETE_DELTA == deleteCount);
#else
    if (g_verbose) printf("\tNegative testing disabled due to lack of "
                           "exception support\n");
#endif
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following functions load a 'bslma::ManagedPtr' object using both a
// factory and a deleter.
// First we perform negative testing when the 'deleter' argument is equal to
// a null pointer.  Note that passing a null pointer literal will produce a
// compile time error in this case, so we store the null in a variable of the
// desired function-pointer type.

template<class POINTER_TYPE, class ObjectPolicy, class FactoryPolicy>
void doConstructObjectFactoryDzero(int callLine, int testLine, int,
                              TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 4 > args->d_config);

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;

    void (*nullFn)(void *, void*) = 0;

    // given a two-argument call to 'load', there is a problem only if
    // 'factory' is null while 'object' has a non-null value, as there is no
    // way to destroy the target object.  Pass a null deleter if that is the
    // goal.
    bool negativeTesting = !nullObject;

    // If we are negative-testing, we will create and destroy any target
    // object entirely within this function, so must track with a local counter
    // instead of the 'args' counter.
    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);

    FactoryType *pAlloc = FactoryPolicy::factory(&ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;


    if (!negativeTesting) {
        ObjectType *pO = 0;
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF, nullFn);

        const bslma::ManagedPtrDeleter del;

        validateManagedState(L_, testObject, 0, del);
    }
    else {
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing null factory pointer\n");

        int deleteCount = 0;
        ObjectType *pO = new(*pAlloc)ObjectType(&deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
        const int expectedCount = ObjectPolicy::DELETE_DELTA;

        bsls::AssertTestHandlerGuard guard;

        ASSERT_SAFE_FAIL_RAW(
                   bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF, nullFn));
        ASSERT_SAFE_FAIL_RAW(
                   bslma::ManagedPtr<POINTER_TYPE> testObject(pO,  0, nullFn));

        pAlloc->deleteObject(pO);
        LOOP2_ASSERT(expectedCount,   deleteCount,
                     expectedCount == deleteCount);
#else
        if (g_verbose) printf("\tNegative testing disabled due to lack of "
                               "exception support\n");
#endif
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Next we supply the actual deleter argument, which now requires three
// separate policies.  Note that the 'deleter' policy is in turn parameterized
// on the types it expects to see, which may be different to (but compatible
// with) the actual 'object' and 'factory' policies used in a given test.

template<class POINTER_TYPE,
         class ObjectPolicy, class FactoryPolicy, class DeleterPolicy>
void doConstructObjectFactoryDeleter(int callLine, int testLine, int index,
                                TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 4 > args->d_config);

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;

    if (nullFactory && FactoryPolicy::DELETER_USES_FACTORY) {
        // It is perfectly well defined to pass a null pointer as the factory
        // if it is not going to be used by the deleter.  We cannot assert
        // this condition in the 'bslma::ManagedPtr' component, so simply exit
        // from this test case, rather than try negative testing strategies.
        // Note that some factory/deleter policies do not actually use the
        // factory argument when running the deleter.  These must be allowed
        // to continue through the rest of this test.
        return;                                                       // RETURN
    }

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);

    FactoryType *pAlloc = FactoryPolicy::factory(&ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    DeleterType *deleter = DeleterPolicy::deleter();

    const int expectedCount = nullObject
                            ? 0
                            : ObjectPolicy::DELETE_DELTA;

    int deleteCount = 0;
    ObjectType *pO = 0;
    if (!nullObject) {
        pO = new(*pAlloc)ObjectType(&deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
    }

    {
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF, deleter);

        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                           pF,
                 reinterpret_cast<bslma::ManagedPtrDeleter::Deleter>(deleter));

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, deleteCount,
                 expectedCount == deleteCount);
}

// Next we supply the actual deleter argument, which now requires three
// separate policies.  Note that the 'deleter' policy is in turn parameterized
// on the types it expects to see, which may be different to (but compatible
// with) the actual 'object' and 'factory' policies used in a given test.

template<class POINTER_TYPE,
         class ObjectPolicy, class FactoryPolicy, class DeleterPolicy>
void doConstructObjectFactoryDeleter2(int callLine, int testLine, int index,
                                      TestCtorArgs *args)
{
    LOOP3_ASSERT(callLine, testLine, args->d_config, 8 > args->d_config);

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;
    bool voidFactory = args->d_config & 4;

    if (nullFactory && FactoryPolicy::DELETER_USES_FACTORY) {
        // It is perfectly well defined to pass a null pointer as the factory
        // if it is not going to be used by the deleter.  We cannot assert
        // this condition in the 'bslma::ManagedPtr' component, so simply exit
        // from this test case, rather than try negative testing strategies.
        // Note that some factory/deleter policies do not actually use the
        // factory argument when running the deleter.  These must be allowed
        // to continue through the rest of this test.
        return;                                                       // RETURN
    }

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);

    FactoryType *pAlloc = FactoryPolicy::factory(&ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    DeleterType *deleter = DeleterPolicy::deleter();

    const int expectedCount = nullObject
                            ? 0
                            : ObjectPolicy::DELETE_DELTA;

    int deleteCount = 0;
    ObjectType *pO = 0;
    if (!nullObject) {
        pO = new(*pAlloc)ObjectType(&deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
    }

    if (!voidFactory)
    {
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, pF, deleter);

        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                          pF,
                                          deleter);

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
    }
    else{
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, (void*)pF, deleter);

        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                          pF,
                                          deleter);

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, deleteCount,
                 expectedCount == deleteCount);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Finally we test the small set of policies that combine to allow passing
// a null pointer literal as the factory.  This requires a deleter that will
// not use the factory pointer.
template<class POINTER_TYPE, class ObjectPolicy, class DeleterPolicy>
void doConstructObjectFnullDeleter(int callLine, int testLine, int index,
                              TestCtorArgs *args)
{
    BSLMF_ASSERT(!DeleterPolicy::DELETER_USES_FACTORY);

    LOOP3_ASSERT(callLine, testLine, args->d_config, 4 > args->d_config);

    bool nullObject  = args->d_config & 1;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    const int expectedCount = nullObject
                            ? 0
                            : ObjectPolicy::DELETE_DELTA;

    int deleteCount = 0;
    ObjectType *pO = 0;
    if (!nullObject) {
        bslma::Allocator *pA = bslma::Default::defaultAllocator();
        pO = new(*pA)ObjectType(&deleteCount);
        args->d_useDefault  = true;
    }

    DeleterType *deleter = DeleterPolicy::deleter();
    {
        bslma::ManagedPtr<POINTER_TYPE> testObject(pO, 0, deleter);

        const bslma::ManagedPtrDeleter del(TestUtil::stripPointerType(pO),
                                           0,
                 reinterpret_cast<bslma::ManagedPtrDeleter::Deleter>(deleter));

        POINTER_TYPE *pTarget = pO;  // implicit cast-to-base etc.
        validateManagedState(L_, testObject, pTarget, del);
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, deleteCount,
                 expectedCount == deleteCount);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// No policies needed for 'load' of empty managed pointers

template <class POINTER_TYPE>
void doLoad(int callLine, int testLine, int index,
            TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const int expectedCount = args->d_deleteDelta;

    args->d_p->load();
    args->d_deleteDelta = 0;

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);

    // As 'd_p' is empty, none of its other properties have a defined state.
}

template <class POINTER_TYPE>
void doLoadOnull(int callLine, int testLine, int index,
                 TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const int expectedCount = args->d_deleteDelta;

    args->d_p->load(0);
    args->d_deleteDelta = 0;

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);

    // As 'd_p' is empty, none of its other properties have a defined state.
}

template <class POINTER_TYPE>
void doLoadOnullFnull(int callLine, int testLine, int index,
                      TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const int expectedCount = args->d_deleteDelta;

    args->d_p->load(0, 0);
    args->d_deleteDelta = 0;

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);

    // As 'd_p' is empty, none of its other properties have a defined state.
}

template <class POINTER_TYPE>
void doLoadOnullFnullDnull(int callLine, int testLine, int index,
                           TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const int expectedCount = args->d_deleteDelta;

// A workaround for early GCC compilers
    args->d_p->load(0, 0, 0);
    args->d_deleteDelta = 0;

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);

    // As 'd_p' is empty, none of its other properties have a defined state.
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// A simple object policy governs loading a single argument

template<class POINTER_TYPE, class ObjectPolicy>
void doLoadObject(int callLine, int testLine, int index,
                  TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const bool nullObject  = args->d_config & 1;

    const int expectedCount = args->d_deleteDelta;

    typedef typename ObjectPolicy::ObjectType ObjectType;

    ObjectType *pO = 0;
    if (nullObject) {
        args->d_p->load(pO);
        args->d_deleteDelta = 0;
    }
    else {
        bslma::Allocator& da = *bslma::Default::defaultAllocator();
        pO = new(da)ObjectType(&args->d_deleteCount);
        args->d_useDefault = true;

        args->d_p->load(pO);
        args->d_deleteDelta = ObjectPolicy::DELETE_DELTA;
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);

    // If we are feeling brave, verify that 'd_p.deleter' has the expected
    // 'object', 'factory' and 'deleter'
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following functions load a 'bslma::ManagedPtr' object using a factory.
// We now require separate policies for Object and Factory types

template<class POINTER_TYPE, class FactoryPolicy>
void doLoadOnullFactory(int callLine, int testLine, int index,
                        TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const int expectedCount = args->d_deleteDelta;

    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    FactoryType *pAlloc = FactoryPolicy::factory(args->d_ta);

    if (FactoryPolicy::USE_DEFAULT) {
        args->d_useDefault = true;
    }

    args->d_p->load(0, pAlloc);
    args->d_deleteDelta = 0;

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP4_ASSERT(callLine, testLine, index, ptr, 0 == ptr);

    // As 'd_p' is empty, none of its other properties have a defined state.
}

template<class POINTER_TYPE, class ObjectPolicy, class FactoryPolicy>
void doLoadObjectFactory(int callLine, int testLine, int index,
                         TestLoadArgs<POINTER_TYPE> *args)
{
    BSLMF_ASSERT(FactoryPolicy::DELETER_USES_FACTORY);

    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    const bool nullObject  = args->d_config & 1;
    const bool nullFactory = args->d_config & 2;

    const int expectedCount = args->d_deleteDelta;

    // given a two-argument call to 'load', there is a problem only if
    // 'factory' is null while 'object' has a non-null value, as there is no
    // way to destroy the target object.  Pass a null deleter if that is the
    // goal.
    bool negativeTesting = !nullObject && nullFactory;

    // If we are negative-testing, we will create and destroy any target
    // object entirely within this function, so must track with a local counter
    // instead of the 'args' counter.
    int deleteCount = 0;

    int * counter = negativeTesting
                  ? &deleteCount
                  : &args->d_deleteCount;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    FactoryType *pAlloc = FactoryPolicy::factory(args->d_ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    ObjectType  *pO = nullObject
                    ? 0
                    : new(*pAlloc)ObjectType(counter);
    if (FactoryPolicy::USE_DEFAULT) {
        args->d_useDefault = true;
    }

    // Load the 'bslma::ManagedPtr' and check that the previous state is
    // correctly cleared.
    if (!negativeTesting) {
        args->d_p->load(pO, pF);
        args->d_deleteDelta = nullObject ? 0 : ObjectPolicy::DELETE_DELTA;

        LOOP5_ASSERT(callLine, testLine, index,
                     expectedCount,   args->d_deleteCount,
                     expectedCount == args->d_deleteCount);

        POINTER_TYPE *ptr = args->d_p->ptr();
        LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);
    }
    else {
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing null factory pointer\n");

        {
            bsls::AssertTestHandlerGuard guard;

            ASSERT_SAFE_FAIL(args->d_p->load(pO, pF));

            pAlloc->deleteObject(pO);

            LOOP_ASSERT(deleteCount,
                        ObjectPolicy::DELETE_DELTA == deleteCount);
        }
#else
        if (g_verbose) printf("\tNegative testing disabled due to lack of "
                               "exception support\n");
#endif
    }

    // If we are feeling brave, verify that 'p.deleter' has the expected
    // 'object', 'factory' and 'deleter'
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following functions load a 'bslma::ManagedPtr' object using both a
// factory and a deleter.
// First we perform negative testing when the 'deleter' argument is equal to
// a null pointer.  Note that passing a null pointer literal will produce a
// compile time error in this case, so we store the null in a variable of the
// desired function-pointer type.

template<class POINTER_TYPE, class ObjectPolicy, class FactoryPolicy>
void doLoadObjectFactoryDzero(int callLine, int testLine, int index,
                              TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;

    void (*nullFn)(void *, void*) = 0;

    const int expectedCount = args->d_deleteDelta;

    // given a two-argument call to 'load', there is a problem only if
    // 'factory' is null while 'object' has a non-null value, as there is no
    // way to destroy the target object.  Pass a null deleter if that is the
    // goal.
    bool negativeTesting = !nullObject;

    // If we are negative-testing, we will create and destroy any target
    // object entirely within this function, so must track with a local counter
    // instead of the 'args' counter.
    int deleteCount = 0;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    FactoryType *pAlloc = FactoryPolicy::factory(args->d_ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    ObjectType *pO = 0;
    if (!nullObject) {
        pO = new(*pAlloc)ObjectType(&deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
    }

    if (!negativeTesting) {
        args->d_p->load(pO, pF, nullFn);
        args->d_deleteDelta = 0;

        LOOP5_ASSERT(callLine, testLine, index,
                     expectedCount,   args->d_deleteCount,
                     expectedCount == args->d_deleteCount);

        POINTER_TYPE *ptr = args->d_p->ptr();
        LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);
    }
    else {
#ifdef BDE_BUILD_TARGET_EXC
        if (g_veryVerbose) printf("\tNegative testing null factory pointer\n");

        {
            bsls::AssertTestHandlerGuard guard;

            ASSERT_SAFE_FAIL(args->d_p->load(pO, pF, nullFn));
            ASSERT_SAFE_FAIL(args->d_p->load(pO,  0, nullFn));

            pAlloc->deleteObject(pO);
            LOOP_ASSERT(deleteCount,
                        ObjectPolicy::DELETE_DELTA == deleteCount);
        }
#else
        if (g_verbose) printf("\tNegative testing disabled due to lack of "
                               "exception support\n");
#endif
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Next we supply the actual deleter argument, which now requires three
// separate policies.  Note that the 'deleter' policy is in turn parameterized
// on the types it expects to see, which may be different to (but compatible
// with) the actual 'object' and 'factory' policies used in a given test.

template<class POINTER_TYPE,
         class ObjectPolicy, class FactoryPolicy, class DeleterPolicy>
void doLoadObjectFactoryDeleter(int callLine, int testLine, int index,
                                TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;

    if (nullFactory && FactoryPolicy::DELETER_USES_FACTORY) {
        // It is perfectly well defined to pass a null pointer as the factory
        // if it is not going to be used by the deleter.  We cannot assert
        // this condition in the 'bslma::ManagedPtr' component, so simply exit
        // from this test case, rather than try negative testing strategies.
        // Note that some factory/deleter policies do not actually use the
        // factory argument when running the deleter.  These must be allowed
        // to continue through the rest of this test.
        return;                                                       // RETURN
    }

    const int expectedCount = args->d_deleteDelta;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    FactoryType *pAlloc = FactoryPolicy::factory(args->d_ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    ObjectType *pO = 0;
    if (!nullObject) {
        pO = new(*pAlloc)ObjectType(&args->d_deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
        args->d_deleteDelta = ObjectPolicy::DELETE_DELTA;
    }

    DeleterType *deleter = DeleterPolicy::deleter();
    args->d_p->load(pO, pF, deleter);

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Next we supply the actual deleter argument, which now requires three
// separate policies.  Note that the 'deleter' policy is in turn parameterized
// on the types it expects to see, which may be different to (but compatible
// with) the actual 'object' and 'factory' policies used in a given test.

template<class POINTER_TYPE,
         class ObjectPolicy, class FactoryPolicy, class DeleterPolicy>
void doLoadObjectFactoryDeleter2(int callLine, int testLine, int index,
                                 TestLoadArgs<POINTER_TYPE> *args)
{
    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    bool nullObject  = args->d_config & 1;
    bool nullFactory = args->d_config & 2;
    bool voidFactory = args->d_config & 4;

    if (nullFactory && FactoryPolicy::DELETER_USES_FACTORY) {
        // It is perfectly well defined to pass a null pointer as the factory
        // if it is not going to be used by the deleter.  We cannot assert
        // this condition in the 'bslma::ManagedPtr' component, so simply exit
        // from this test case, rather than try negative testing strategies.
        // Note that some factory/deleter policies do not actually use the
        // factory argument when running the deleter.  These must be allowed
        // to continue through the rest of this test.
        return;                                                       // RETURN
    }

    const int expectedCount = args->d_deleteDelta;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename FactoryPolicy::FactoryType FactoryType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    // We need two factory pointers, 'pAlloc' is used for all necessary
    // allocations and destructions within this function, while 'pF' is the
    // factory pointer passed to load, which is either the same as 'pAlloc' or
    // null.
    FactoryType *pAlloc = FactoryPolicy::factory(args->d_ta);
    FactoryType *pF = nullFactory
                    ? 0
                    : pAlloc;

    ObjectType *pO = 0;
    if (!nullObject) {
        pO = new(*pAlloc)ObjectType(&args->d_deleteCount);
        if (FactoryPolicy::USE_DEFAULT) {
            args->d_useDefault = true;
        }
        args->d_deleteDelta = ObjectPolicy::DELETE_DELTA;
    }

    DeleterType *deleter = DeleterPolicy::deleter();
    if (!voidFactory) {
        args->d_p->load(pO, pF, deleter);
    }
    else {
        args->d_p->load(pO, (void*)pF, deleter);
    }

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Finally we test the small set of policies that combine to allow passing
// a null pointer literal as the factory.  This requires a deleter that will
// not use the factory pointer.
template<class POINTER_TYPE, class ObjectPolicy, class DeleterPolicy>
void doLoadObjectFnullDeleter(int callLine, int testLine, int index,
                              TestLoadArgs<POINTER_TYPE> *args)
{
    BSLMF_ASSERT(!DeleterPolicy::DELETER_USES_FACTORY);

    validateTestLoadArgs(callLine, testLine, args); // Assert pre-conditions

    bool nullObject  = args->d_config & 1;

    const int expectedCount = args->d_deleteDelta;

    typedef typename  ObjectPolicy::ObjectType  ObjectType;
    typedef typename DeleterPolicy::DeleterType DeleterType;

    ObjectType *pO = 0;
    if (!nullObject) {
        bslma::Allocator *pA = bslma::Default::defaultAllocator();
        pO = new(*pA)ObjectType(&args->d_deleteCount);
        args->d_useDefault  = true;
        args->d_deleteDelta = ObjectPolicy::DELETE_DELTA;
    }

    DeleterType *deleter = DeleterPolicy::deleter();
    args->d_p->load(pO, 0, deleter);

    LOOP5_ASSERT(callLine, testLine, index, expectedCount, args->d_deleteCount,
                 expectedCount == args->d_deleteCount);

    POINTER_TYPE *ptr = args->d_p->ptr();
    LOOP5_ASSERT(callLine, testLine, index, pO, ptr, pO == ptr);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Once all the testing policies are composed into arrays of test functions, we
// need some driver functions to iterate over each valid combination (stored
// in separate test tables) and check that the behavior transitions correctly
// in each case.
struct NullPolicy {};

template<class TARGET>
struct TestPolicy {
    typedef void TestLoadFn(int, int, int, TestLoadArgs<TARGET> *);
    typedef void TestCtorFn(int, int, int, TestCtorArgs *);

    TestLoadFn  *testLoad;
    TestCtorFn  *testCtor;

    int          d_configs;
    unsigned     configs() const { return d_configs; }
//    unsigned     configs() const { return 4; }

    TestPolicy()
    : testLoad(&doLoad     <TARGET>)
    , testCtor(&doConstruct<TARGET>)
    , d_configs(1)
    {
    }

    explicit TestPolicy(NullPolicy)
    : testLoad(&doLoadOnull     <TARGET>)
    , testCtor(&doConstructOnull<TARGET>)
    , d_configs(1)
    {
    }

    TestPolicy(NullPolicy, NullPolicy)
    : testLoad(&doLoadOnullFnull     <TARGET>)
    , testCtor(&doConstructOnullFnull<TARGET>)
    , d_configs(1)
    {
    }

    TestPolicy(NullPolicy, NullPolicy, NullPolicy)
    : testLoad(&doLoadOnullFnullDnull     <TARGET>)
    , testCtor(&doConstructOnullFnullDnull<TARGET>)
    , d_configs(1)
    {
    }

    template<class ObjectPolicy>
    explicit TestPolicy(ObjectPolicy)
    : testLoad(&doLoadObject     <TARGET, ObjectPolicy>)
    , testCtor(&doConstructObject<TARGET, ObjectPolicy>)
    , d_configs(2)
    {
    }

    template<class FactoryPolicy>
    TestPolicy(NullPolicy, FactoryPolicy)
    : testLoad(&doLoadOnullFactory     <TARGET, FactoryPolicy>)
    , testCtor(&doConstructOnullFactory<TARGET, FactoryPolicy>)
    , d_configs(1)
    {
    }

    template<class ObjectPolicy, class FactoryPolicy>
    TestPolicy(ObjectPolicy, FactoryPolicy)
    : testLoad(&doLoadObjectFactory     <TARGET, ObjectPolicy, FactoryPolicy>)
    , testCtor(&doConstructObjectFactory<TARGET, ObjectPolicy, FactoryPolicy>)
    , d_configs(4)
    {
    }

    template<class ObjectPolicy, class FactoryPolicy, class DeleterPolicy>
    TestPolicy(ObjectPolicy, FactoryPolicy, DeleterPolicy)
    : testLoad(&doLoadObjectFactoryDeleter
                          <TARGET, ObjectPolicy, FactoryPolicy, DeleterPolicy>)
    , testCtor(&doConstructObjectFactoryDeleter
                          <TARGET, ObjectPolicy, FactoryPolicy, DeleterPolicy>)
    , d_configs(4)
    {
    }

    template<class ObjectPolicy, class FactoryPolicy,
             class DeleterObjectPolicy, class DeleterFactoryPolicy>
    TestPolicy(ObjectPolicy,
               FactoryPolicy,
               DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy>)
    : testLoad(&doLoadObjectFactoryDeleter2<
                        TARGET, ObjectPolicy, FactoryPolicy,
                        DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy> >)
    , testCtor(&doConstructObjectFactoryDeleter2
                       <TARGET, ObjectPolicy, FactoryPolicy,
                        DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy> >)
    , d_configs(8)
    {
    }

    template<class ObjectPolicy, class DeleterPolicy>
    TestPolicy(ObjectPolicy, NullPolicy, DeleterPolicy)
    : testLoad(&doLoadObjectFnullDeleter <TARGET, ObjectPolicy, DeleterPolicy>)
    , testCtor(&doConstructObjectFnullDeleter
                                         <TARGET, ObjectPolicy, DeleterPolicy>)
    , d_configs(2)
    {
    }

    template<class ObjectPolicy,
             class DeleterObjectPolicy,
             class DeleterFactoryPolicy>
    TestPolicy(ObjectPolicy,
               NullPolicy,
               DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy>)
    : testLoad(&doLoadObjectFnullDeleter
                       <TARGET, ObjectPolicy,
                        DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy> >)
    , testCtor(&doConstructObjectFnullDeleter
                       <TARGET, ObjectPolicy,
                        DVoidVoid<DeleterObjectPolicy, DeleterFactoryPolicy> >)
    , d_configs(2)
    {
    }

    template<class ObjectPolicy, class FactoryPolicy>
    TestPolicy(ObjectPolicy, FactoryPolicy, NullPolicy)
    : testLoad(&doLoadObjectFactoryDzero <TARGET, ObjectPolicy, FactoryPolicy>)
    , testCtor(&doConstructObjectFactoryDzero
                                         <TARGET, ObjectPolicy, FactoryPolicy>)
    , d_configs(4)
    {
    }
};


typedef void (*TestCtorFn)(int, int, int, TestCtorArgs *);

template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testConstructors(int callLine,
                  const TestPolicy<TEST_TARGET>(&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function iterates all viable variations of test functions composed
    // of the policies above, to verify that all 'bslma::ManagedPtr::load'
    // behave according to contract.  First, we call 'load' on an empty managed
    // pointer using a test function from the passed array, confirming that
    // the managed pointer takes up the correct state.  Then we allow that
    // pointer to go out of scope, and confirm that any managed object is
    // destroyed using the correct deleter.  Next we repeat the test, setting
    // up the same, now well-known, state of the managed pointer, and replace
    // it with a second call to load (by a second iterator over the array of
    // test functions).  We confirm that the original state and managed object
    // (if any) are destroyed correctly, and that the expected new state has
    // been established correctly.  Finally, we allow this pointer to leave
    // scope and confirm that all managed objects are destroyed correctly and
    // all allocated memory has been reclaimed.  At each stage, we perform
    // negative testing where appropriate, and check that no memory is being
    // allocated other than by the object allocator, or the default allocator
    // only for those test functions that return a state indicating that they
    // used the default allocator.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator* ga = dynamic_cast<bslma::TestAllocator *>
                                           (bslma::Default::globalAllocator());

    bslma::TestAllocator* da = dynamic_cast<bslma::TestAllocator *>
                                          (bslma::Default::defaultAllocator());

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned config = 0; config != TEST_ARRAY[i].configs(); ++config)
        {
            TestCtorArgs args = { false, config };

            bslma::TestAllocatorMonitor gam(ga);
            bslma::TestAllocatorMonitor dam(da);

            args.d_useDefault = false;

            TEST_ARRAY[i].testCtor(callLine, L_, i, &args);

            LOOP2_ASSERT(L_, i, gam.isInUseSame());
            LOOP2_ASSERT(L_, i, gam.isMaxSame());

            LOOP2_ASSERT(L_, i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP2_ASSERT(L_, i, dam.isMaxSame());
            }
        }
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testLoadOps(int callLine,
                 const TestPolicy<TEST_TARGET> (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function iterates all viable variations of test functions composed
    // of the policies above, to verify that all 'bslma::ManagedPtr::load'
    // behave according to contract.  First, we call 'load' on an empty managed
    // pointer using a test function from the passed array, confirming that
    // the managed pointer takes up the correct state.  Then we allow that
    // pointer to go out of scope, and confirm that any managed object is
    // destroyed using the correct deleter.  Next we repeat the test, setting
    // up the same, now well-known, state of the managed pointer, and replace
    // it with a second call to load (by a second iterator over the array of
    // test functions).  We confirm that the original state and managed object
    // (if any) are destroyed correctly, and that the expected new state has
    // been established correctly.  Finally, we allow this pointer to leave
    // scope and confirm that all managed objects are destroyed correctly and
    // all allocated memory has been reclaimed.  At each stage, we perform
    // negative testing where appropriate, and check that no memory is being
    // allocated other than by the object allocator, or the default allocator
    // only for those test functions that return a state indicating that they
    // used the default allocator.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    TestLoadArgs<TEST_TARGET> args = {};

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned configI = 0; configI != TEST_ARRAY[i].configs();
                                                                   ++configI) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            args.d_useDefault = false;
            args.d_config = configI;

            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i].testLoad(callLine, L_, i, &args);
            }
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }

            for (int j = 0; j != TEST_ARRAY_SIZE; ++j) {
                for (unsigned configJ = 0; configJ != TEST_ARRAY[j].configs();
                                                                   ++configJ) {
                    bslma::TestAllocatorMonitor dam2(&da);

                    bslma::TestAllocator ta("TestLoad 2",
                                            g_veryVeryVeryVerbose);

                    TestPointer p;
                    ASSERT(0 == p.ptr());

                    args.d_p  = &p;
                    args.d_ta = &ta;
                    args.d_config = configI;

                    args.d_deleteCount = 0;
                    args.d_deleteDelta = 0;
                    args.d_useDefault  = false;
                    TEST_ARRAY[i].testLoad(callLine, L_, i, &args);

                    args.d_config = configJ;
                    args.d_deleteCount = 0;
                    TEST_ARRAY[j].testLoad(callLine, L_, j, &args);

                    // Clear 'deleteCount' before 'p' is destroyed.
                    args.d_deleteCount = 0;

                    LOOP_ASSERT(i, gam.isInUseSame());
                    LOOP_ASSERT(i, gam.isMaxSame());

                    if (!args.d_useDefault) {
                        LOOP_ASSERT(i, dam2.isInUseSame());
                        LOOP_ASSERT(i, dam2.isMaxSame());
                    }
                }
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, dam.isInUseSame());
            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());
        }
    }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
template <class T>
struct AliasTestType1 {
    typedef MyDerivedObject type;
};

template <class T>
struct AliasTestType2 {
    typedef MySecondDerivedObject type;
};

template <>
struct AliasTestType1<Base> {
    typedef Base2 type;
};

template <>
struct AliasTestType2<Base> {
    typedef Composite type;
};

template <>
struct AliasTestType1<Base2> {
    typedef Base2 type;
};

template <>
struct AliasTestType2<Base2> {
    typedef Composite type;
};


template <class T>
struct AliasTestType2<const T> : AliasTestType2<T> {};

template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testLoadAliasOps1(int callLine,
                  const TestPolicy<TEST_TARGET> (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function validates the simple scenario of calling 'loadAlias' to
    // create a simple aliased managed pointer, and confirming that pointer
    // destroyed its managed object with the correct deleter and reclaims all
    // memory when destroyed by leaving scope.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    int aliasDeleterCount = 0;
    typename AliasTestType1<TEST_TARGET>::type aliasTarget(&aliasDeleterCount);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned configI = 0; configI != TEST_ARRAY[i].configs();
                                                                   ++configI) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            TestLoadArgs<TEST_TARGET> args = {};
            args.d_useDefault = false;
            args.d_config = configI;

            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i].testLoad(callLine, L_, i, &args);

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'mode' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

#ifdef BDE_BUILD_TARGET_EXC
                if (g_veryVerbose) printf(
                                         "\tNegative testing null pointers\n");

                TestPointer pAlias;
                if (0 == p.ptr()) {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias.loadAlias(p, &aliasTarget));
                    ASSERT_SAFE_PASS(pAlias.loadAlias(p, 0));

                    LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                    LOOP_ASSERT(pAlias.ptr(), 0 == pAlias.ptr());
                }
                else {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias.loadAlias(p, 0));
                    ASSERT_SAFE_PASS(pAlias.loadAlias(p, &aliasTarget));

                    LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                    LOOP_ASSERT(pAlias.ptr(), &aliasTarget == pAlias.ptr());
                }
#else
                TestPointer pAlias;
                TEST_TARGET *pTarget = 0 == p.ptr()
                                     ? 0
                                     : &aliasTarget;

                pAlias.loadAlias(p, pTarget);

                LOOP_ASSERT(p.ptr(),  0 == p.ptr());
                LOOP2_ASSERT(pTarget, pAlias.ptr(), pTarget == pAlias.ptr());
#endif

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}

template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testLoadAliasOps2(int callLine,
                  const TestPolicy<TEST_TARGET> (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This scenario tests the correct state change for following a 'loadAlias'
    // call with another 'loadAlias' call.  It will also test derived* -> base*
    // conversions for the aliased pointer, and non-const* -> const*.  The test
    // process is to take an empty 'bslma::ManagedPtr' object and 'load' a
    // known state into it using a well-known test function.  Then we "alias"
    // this pointer by calling 'loadAlias' on another (empty) managed pointer
    // object, and check that the new aliased state has been created correctly,
    // without allocating any memory, and that the original managed pointer
    // object is now empty.  Next we establish another well-known managed
    // pointer value, and call 'loadAlias' again on the pointer in the existing
    // aliased state.  We again confirm that the aliased state is transferred
    // without allocating any memory, but also that the object managed by the
    // original 'bslma::ManagedPtr' object has now been destroyed as expected.
    // Finally we let this final managed pointer object leave scope and confirm
    // that all managed objects have been destroyed, as expected, and that all
    // memory has been reclaimed.  At each step, we further implement negative
    // testing if a null pointer may be passed, and that passing a null pointer
    // would yield (negatively testable) undefined behavior.

    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    TestLoadArgs<TEST_TARGET> args = {};

    int aliasDeleterCount1 = 0;
    int aliasDeleterCount2 = 0;
    typename AliasTestType1<TEST_TARGET>::type alias1(&aliasDeleterCount1);
    typename AliasTestType2<TEST_TARGET>::type alias2(&aliasDeleterCount2);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned configI = 0; configI != TEST_ARRAY[i].configs();
                                                                   ++configI) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            args.d_useDefault = false;
            args.d_config = configI;

            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i].testLoad(callLine, L_, i, &args);

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'mode' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

#ifdef BDE_BUILD_TARGET_EXC
                if (g_veryVerbose) printf(
                                         "\tNegative testing null pointers\n");

                // Declare variables so that the lifetime extends to the end
                // of the loop.  Otherwise, the 'ta' monitor tests will flag
                // the 'pAlias2' destructor for freeing the original object.
                TestPointer pAlias1;
                TestPointer pAlias2;

                if (0 == p.ptr()) {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias1.loadAlias(p, &alias1));
                    ASSERT_SAFE_PASS(pAlias1.loadAlias(p, 0));

                    LOOP_ASSERT(p.ptr(),       0 == p.ptr());
                    LOOP_ASSERT(pAlias1.ptr(), 0 == pAlias1.ptr());
                }
                else {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias1.loadAlias(p, 0));
                    ASSERT_SAFE_PASS(pAlias1.loadAlias(p, &alias1));

                    LOOP_ASSERT(p.ptr(), 0 == p.ptr());
                    LOOP2_ASSERT(&alias1,   pAlias1.ptr(),
                                 &alias1 == pAlias1.ptr());

                    ASSERT_SAFE_FAIL(pAlias2.loadAlias(pAlias1, 0));
                    ASSERT_SAFE_PASS(pAlias2.loadAlias(pAlias1, &alias2));

                    LOOP_ASSERT(pAlias1.ptr(), 0 == pAlias1.ptr());
                    LOOP2_ASSERT(&alias2,   pAlias2.ptr(),
                                 &alias2 == pAlias2.ptr());
                }
#else
                TestPointer pAlias1;
                TEST_TARGET *pTarget = 0 == p.ptr()
                                     ? 0
                                     : &alias1;

                pAlias1.loadAlias(p, pTarget);

                LOOP_ASSERT(p.ptr(),  0 == p.ptr());
                LOOP2_ASSERT(pTarget, pAlias1.ptr(), pTarget == pAlias1.ptr());
#endif

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}


template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testLoadAliasOps3(int callLine,
                   const TestPolicy<TEST_TARGET>(&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function tests the correct interaction of 'load' and 'loadAlias'.
    // Initially, an empty 'bslma::ManagedPtr' object is loaded into a well
    // defined non-empty state using a well-known test loader.  This state is
    // then transferred to a second empty pointer through a 'loadAlias' call,
    // and we validate that no memory is allocated for this operation, and the
    // state is correctly transferred.  Next we replace this aliased state with
    // another well-known state using 'load' again.  We test that the initial
    // state is correctly destroyed, and the new state is in place without any
    // aliasing.  Then we allow this final state to be destroyed, and confirm
    // that all managed objects have been correctly disposed of.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    int aliasDeleterCount = 0;
    typename AliasTestType1<TEST_TARGET>::type aliasTarget(&aliasDeleterCount);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (int j = 0; j != TEST_ARRAY_SIZE; ++j) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            TestLoadArgs<TEST_TARGET> args = {};
            args.d_useDefault = false;
            args.d_config = 0;  // We need only test a fully defined pointer,
                                // there are no concerns about null arguments.
            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i].testLoad(callLine, L_, i, &args);
                if (0 == p.ptr()) {
                    // We have no interest in tests that create a null pointer,
                    // this scenario is negative tested in testLoadAliasOps1.
                    continue;
                }

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'move' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

                TestPointer pAlias;
                pAlias.loadAlias(p, &aliasTarget);

                LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                LOOP_ASSERT(pAlias.ptr(), &aliasTarget == pAlias.ptr());

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());

                // Next we load a fresh state into the pointer to verify the
                // final concern for 'load'; that it correctly destroys an
                // aliased state while acquire the new value.
                args.d_p  = &pAlias;

                // The test function itself asserts correct destructor count
                // for this transition, and that the 'pAlias' has the correct
                // final state.
                TEST_ARRAY[j].testLoad(callLine, L_, j, &args);

                LOOP_ASSERT(i, gam.isInUseSame());
                LOOP_ASSERT(i, gam.isMaxSame());

                if (!args.d_useDefault) {
                    LOOP_ASSERT(i, dam.isInUseSame());
                    LOOP_ASSERT(i, dam.isMaxSame());
                }

                // Nothing further to assert, but reset 'deleteCount' to
                // verify destroying final objects outside the loop.
                args.d_deleteCount = 0;
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}

template<class TEST_TARGET, size_t TEST_ARRAY_SIZE>
void testConstructors(int callLine,
                      const TestCtorFn(&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function iterates all viable variations of test functions composed
    // of the policies above, to verify that all 'bslma::ManagedPtr::load'
    // behave according to contract.  First, we call 'load' on an empty managed
    // pointer using a test function from the passed array, confirming that
    // the managed pointer takes up the correct state.  Then we allow that
    // pointer to go out of scope, and confirm that any managed object is
    // destroyed using the correct deleter.  Next we repeat the test, setting
    // up the same, now well-known, state of the managed pointer, and replace
    // it with a second call to load (by a second iterator over the array of
    // test functions).  We confirm that the original state and managed object
    // (if any) are destroyed correctly, and that the expected new state has
    // been established correctly.  Finally, we allow this pointer to leave
    // scope and confirm that all managed objects are destroyed correctly and
    // all allocated memory has been reclaimed.  At each stage, we perform
    // negative testing where appropriate, and check that no memory is being
    // allocated other than by the object allocator, or the default allocator
    // only for those test functions that return a state indicating that they
    // used the default allocator.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator* ga = dynamic_cast<bslma::TestAllocator *>
                                           (bslma::Default::globalAllocator());

    bslma::TestAllocator* da = dynamic_cast<bslma::TestAllocator *>
                                          (bslma::Default::defaultAllocator());

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned config = 0; config != TEST_ARRAY[i].configs(); ++config)
        {
            TestCtorArgs args = { false, config};

            bslma::TestAllocatorMonitor gam(ga);
            bslma::TestAllocatorMonitor dam(da);

            args.d_useDefault = false;

            TEST_ARRAY[i](callLine, L_, i, &args);

            LOOP2_ASSERT(L_, i, gam.isInUseSame());
            LOOP2_ASSERT(L_, i, gam.isMaxSame());

            LOOP2_ASSERT(L_, i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP2_ASSERT(L_, i, dam.isMaxSame());
            }
        }
    }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class TEST_TARGET,
         class TEST_FUNCTION_TYPE,
         size_t TEST_ARRAY_SIZE>
void testLoadAliasOps1(int callLine,
                       const TEST_FUNCTION_TYPE (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function validates the simple scenario of calling 'loadAlias' to
    // create a simple aliased managed pointer, and confirming that pointer
    // destroyed its managed object with the correct deleter and reclaims all
    // memory when destroyed by leaving scope.
    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    int aliasDeleterCount = 0;
    typename AliasTestType1<TEST_TARGET>::type aliasTarget(&aliasDeleterCount);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned configI = 0; configI != TEST_ARRAY[i].configs();
                                                                   ++configI) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            TestLoadArgs<TEST_TARGET> args = {};
            args.d_useDefault = false;
            args.d_config = configI;

            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i](callLine, L_, i, &args);

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'mode' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

#ifdef BDE_BUILD_TARGET_EXC
                if (g_veryVerbose)
                                  printf("\tNegative testing null pointers\n");

                TestPointer pAlias;
                if (0 == p.ptr()) {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias.loadAlias(p, &aliasTarget));
                    ASSERT_SAFE_PASS(pAlias.loadAlias(p, 0));

                    LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                    LOOP_ASSERT(pAlias.ptr(), 0 == pAlias.ptr());
                }
                else {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias.loadAlias(p, 0));
                    ASSERT_SAFE_PASS(pAlias.loadAlias(p, &aliasTarget));

                    LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                    LOOP_ASSERT(pAlias.ptr(), &aliasTarget == pAlias.ptr());
                }
#else
                TestPointer pAlias;
                TEST_TARGET pTarget = 0 == p.ptr()
                                    ? 0
                                    : &aliasTarget;

                pAlias.loadAlias(p, pTarget);

                LOOP_ASSERT(p.ptr(),  0 == p.ptr());
                LOOP2_ASSERT(pTarget, pAlias.ptr(), pTarget == pAlias.ptr());
#endif

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}

template<class TEST_TARGET,
         class TEST_FUNCTION_TYPE,
         size_t TEST_ARRAY_SIZE>
void testLoadAliasOps2(int callLine,
                       const TEST_FUNCTION_TYPE (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This scenario tests the correct state change for following a 'loadAlias'
    // call with another 'loadAlias' call.  It will also test derived* -> base*
    // conversions for the aliased pointer, and non-const* -> const*.  The test
    // process is to take an empty 'bslma::ManagedPtr' object and 'load' a
    // known state into it using a well-known test function.  Then we "alias"
    // this pointer by calling 'loadAlias' on another (empty) managed pointer
    // object, and check that the new aliased state has been created correctly,
    // without allocating any memory, and that the original managed pointer
    // object is now empty.  Next we establish another well-known managed
    // pointer value, and call 'loadAlias' again on the pointer in the existing
    // aliased state.  We again confirm that the aliased state is transferred
    // without allocating any memory, but also that the object managed by the
    // original 'bslma::ManagedPtr' object has now been destroyed as expected.
    // Finally we let this final managed pointer object leave scope and confirm
    // that all managed objects have been destroyed, as expected, and that all
    // memory has been reclaimed.  At each step, we further implement negative
    // testing if a null pointer may be passed, and that passing a null pointer
    // would yield (negatively testable) undefined behavior.

    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    TestLoadArgs<TEST_TARGET> args = {};

    int aliasDeleterCount1 = 0;
    int aliasDeleterCount2 = 0;
    typename AliasTestType1<TEST_TARGET>::type alias1(&aliasDeleterCount1);
    typename AliasTestType2<TEST_TARGET>::type alias2(&aliasDeleterCount2);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (unsigned configI = 0; configI != TEST_ARRAY[i].configs();
                                                                   ++configI) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            args.d_useDefault = false;
            args.d_config = configI;

            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i](callLine, L_, i, &args);

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'mode' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

#ifdef BDE_BUILD_TARGET_EXC
                if (g_veryVerbose)
                                  printf("\tNegative testing null pointers\n");

                // Declare variables so that the lifetime extends to the end
                // of the loop.  Otherwise, the 'ta' monitor tests will flag
                // the 'pAlias2' destructor for freeing the original object.
                TestPointer pAlias1;
                TestPointer pAlias2;

                if (0 == p.ptr()) {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias1.loadAlias(p, &alias1));
                    ASSERT_SAFE_PASS(pAlias1.loadAlias(p, 0));

                    LOOP_ASSERT(p.ptr(),       0 == p.ptr());
                    LOOP_ASSERT(pAlias1.ptr(), 0 == pAlias1.ptr());
                }
                else {
                    bsls::AssertTestHandlerGuard guard;

                    ASSERT_SAFE_FAIL(pAlias1.loadAlias(p, 0));
                    ASSERT_SAFE_PASS(pAlias1.loadAlias(p, &alias1));

                    LOOP_ASSERT(p.ptr(), 0 == p.ptr());
                    LOOP2_ASSERT(&alias1,   pAlias1.ptr(),
                                 &alias1 == pAlias1.ptr());

                    ASSERT_SAFE_FAIL(pAlias2.loadAlias(pAlias1, 0));
                    ASSERT_SAFE_PASS(pAlias2.loadAlias(pAlias1, &alias2));

                    LOOP_ASSERT(pAlias1.ptr(), 0 == pAlias1.ptr());
                    LOOP2_ASSERT(&alias2,   pAlias2.ptr(),
                                 &alias2 == pAlias2.ptr());
                }
#else
                TestPointer pAlias1;
                TEST_TARGET pTarget = 0 == p.ptr()
                                    ? 0
                                    : &alias1;

                pAlias1.loadAlias(p, pTarget);

                LOOP_ASSERT(p.ptr(),  0 == p.ptr());
                LOOP2_ASSERT(pTarget, pAlias1.ptr(), pTarget == pAlias1.ptr());
#endif

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}


template<class TEST_TARGET,
         class TEST_FUNCTION_TYPE,
         size_t TEST_ARRAY_SIZE>
void testLoadAliasOps3(int callLine,
                       const TEST_FUNCTION_TYPE (&TEST_ARRAY)[TEST_ARRAY_SIZE])
{
    // This function tests the correct interaction of 'load' and 'loadAlias'.
    // Initially, an empty 'bslma::ManagedPtr' object is loaded into a well
    // defined non-empty state using a well-known test loader.  This state is
    // then transferred to a second empty pointer through a 'loadAlias' call,
    // and we validate that no memory is allocated for this operation, and the
    // state is correctly transferred.  Next we replace this aliased state with
    // another well-known state using 'load' again.  We test that the initial
    // state is correctly destroyed, and the new state is in place without any
    // aliasing.  Then we allow this final state to be destroyed, and confirm
    // that all managed objects have been correctly disposed of.

    typedef bslma::ManagedPtr<TEST_TARGET> TestPointer;

    bslma::TestAllocator& ga = dynamic_cast<bslma::TestAllocator&>
                                          (*bslma::Default::globalAllocator());

    bslma::TestAllocator& da = dynamic_cast<bslma::TestAllocator&>
                                         (*bslma::Default::defaultAllocator());

    int aliasDeleterCount = 0;
    typename AliasTestType1<TEST_TARGET>::type aliasTarget(&aliasDeleterCount);

    for (int i = 0; i != TEST_ARRAY_SIZE; ++i) {
        for (int j = 0; j != TEST_ARRAY_SIZE; ++j) {
            bslma::TestAllocatorMonitor gam(&ga);
            bslma::TestAllocatorMonitor dam(&da);

            TestLoadArgs<TEST_TARGET> args = {};
            args.d_useDefault = false;
            args.d_config = 0;  // We need only test a fully defined pointer,
                                // there are no concerns about null arguments.
            {
                bslma::TestAllocator ta("TestLoad 1", g_veryVeryVeryVerbose);
                TestPointer p;
                ASSERT(0 == p.ptr());

                args.d_p  = &p;
                args.d_ta = &ta;

                args.d_deleteCount = 0;
                args.d_deleteDelta = 0;
                TEST_ARRAY[i](callLine, L_, i, &args);
                if (0 == p.ptr()) {
                    // We have no interest in tests that create a null pointer,
                    // this scenario is negative tested in testLoadAliasOps1.
                    continue;
                }

                // Check that no more memory is allocated or freed.
                // All operations from here are effectively 'move' operations.
                bslma::TestAllocatorMonitor gam2(&ga);
                bslma::TestAllocatorMonitor dam2(&da);
                bslma::TestAllocatorMonitor tam2(&ta);

                TestPointer pAlias;
                pAlias.loadAlias(p, &aliasTarget);

                LOOP_ASSERT(p.ptr(),      0 == p.ptr());
                LOOP_ASSERT(pAlias.ptr(), &aliasTarget == pAlias.ptr());

                // Assert that no memory was allocated or freed
                LOOP_ASSERT(i, tam2.isInUseSame());
                LOOP_ASSERT(i, tam2.isMaxSame());
                LOOP_ASSERT(i, dam2.isInUseSame());
                LOOP_ASSERT(i, dam2.isMaxSame());
                LOOP_ASSERT(i, gam2.isInUseSame());
                LOOP_ASSERT(i, gam2.isMaxSame());

                // Next we load a fresh state into the pointer to verify the
                // final concern for 'load'; that it correctly destroys an
                // aliased state while acquire the new value.
                args.d_p  = &pAlias;

                // The test function itself asserts correct destructor count
                // for this transition, and that the 'pAlias' has the correct
                // final state.
                TEST_ARRAY[j](callLine, L_, j, &args);

                LOOP_ASSERT(i, gam.isInUseSame());
                LOOP_ASSERT(i, gam.isMaxSame());

                if (!args.d_useDefault) {
                    LOOP_ASSERT(i, dam.isInUseSame());
                    LOOP_ASSERT(i, dam.isMaxSame());
                }

                // Nothing further to assert, but reset 'deleteCount' to
                // verify destroying final objects outside the loop.
                args.d_deleteCount = 0;
            }

            // Validate the final deleter run when 'p' is destroyed.
            LOOP2_ASSERT(args.d_deleteCount,   args.d_deleteDelta,
                         args.d_deleteCount == args.d_deleteDelta);

            LOOP_ASSERT(i, gam.isInUseSame());
            LOOP_ASSERT(i, gam.isMaxSame());

            LOOP_ASSERT(i, dam.isInUseSame());
            if (!args.d_useDefault) {
                LOOP_ASSERT(i, dam.isMaxSame());
            }
        }
    }
}

//=============================================================================
// This is the test table for iterating constructor and load functions for
// 'bslma::ManagedPtr<MyTestObject>'.  The same test table is created for each
// of the main 5 tested pointer types, and then the invalid functions are
// commented out, to audit that they have intentionally been reviewed and
// rejected.  This allows us to compare the different test tables if a
// discrepancy occurs in the future.
// In particular, this case does not support construction from pointers to
// 'const' objects.

static const TestPolicy<MyTestObject> TEST_POLICY_BASE_ARRAY[] = {
    // default test
    TestPolicy<MyTestObject>(),

    // single object-pointer tests
    TestPolicy<MyTestObject>( NullPolicy() ),
    TestPolicy<MyTestObject>( NullPolicy(), NullPolicy() ),
    TestPolicy<MyTestObject>( NullPolicy(), NullPolicy(), NullPolicy() ),

    TestPolicy<MyTestObject>( Obase() ),
    TestPolicy<MyTestObject>( Oderiv() ),
    //TestPolicy<MyTestObject>( OCbase() ),
    //TestPolicy<MyTestObject>( OCderiv() ),

    // factory tests

    TestPolicy<MyTestObject>( NullPolicy(), Ftst() ),
    TestPolicy<MyTestObject>( NullPolicy(), Fbsl() ),
    TestPolicy<MyTestObject>( Obase(),      Ftst() ),
    TestPolicy<MyTestObject>( Obase(),      Fbsl() ),
    TestPolicy<MyTestObject>( Oderiv(),     Ftst() ),
    TestPolicy<MyTestObject>( Oderiv(),     Fbsl() ),
    //TestPolicy<MyTestObject>( OCbase(),   Ftst() ),
    //TestPolicy<MyTestObject>( OCbase(),   Fbsl() ),
    //TestPolicy<MyTestObject>( OCderiv(),  Ftst() ),
    //TestPolicy<MyTestObject>( OCderiv(),  Fbsl() ),
    // deleter tests

    // First test the non-deprecated interface, using the policy
    // 'DVoidVoid'.

    // MyTestObject
    TestPolicy<MyTestObject>( Obase(), Ftst(), DVoidVoid< Obase,   Ftst >() ),
    TestPolicy<MyTestObject>( Obase(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<MyTestObject>( Obase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyTestObject
    //TestPolicy<MyTestObject>( OCbase(), Ftst(), DVoidVoid< OCbase, Ftst >() ),
    //TestPolicy<MyTestObject>( OCbase(), Fbsl(), DVoidVoid< OCbase, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCbase(), Ftst(), DVoidVoid< OCbase, Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<MyTestObject>( Obase(),  Fdflt(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<MyTestObject>( Obase(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),


    // Next we test the deprecated support for deleters other than
    // 'void (*)(void *, void *)', starting with deleters that
    // type-erase the 'object' type, but have a strongly typed
    // 'factory' argument.  Such deleters are generated by the
    // 'DVoidFac' policy..

    // MyTestObject
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DVoidFac< Obase,   Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyTestObject
    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),
    //TestPolicy<MyTestObject>( OCbase(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Ftst >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Ftst >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DVoidFac<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<MyTestObject>( Obase(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<MyTestObject>( Obase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<Oderiv,  Fdflt>() ),
    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),

    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    // HERE WE ARE DOUBLY-BROKEN AS CV-QUALIFIED TYPES ARE NOT
    // SUPPORTED FOR TYPE-ERASURE THROUGH DELETER
    //TestPolicy<MyTestObject>( OCbase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),


    // Now we test deleters that are strongly typed for the
    // 'object' parameter, but type-erase the 'factory'.

    // MyTestObject
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjVoid< Obase,   Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyTestObject
    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),
    //TestPolicy<MyTestObject>( OCbase(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Ftst >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Ftst >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<MyTestObject>( Obase(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),


    // Finally we test the most generic combination of generic
    // object type, a factory, and a deleter taking two arguments
    // compatible with pointers to the invoking 'object' and
    // 'factory' types.

    // MyTestObject
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjFac< Obase,   Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Obase(),   Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyTestObject
    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Ftst >() ),
    //TestPolicy<MyTestObject>( OCbase(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Ftst >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Ftst >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DObjFac<Obase,   Fdflt>() ),
    TestPolicy<MyTestObject>( Obase(),   Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjFac<Oderiv,  Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjFac<Obase,   Fdflt>() ),

    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<MyTestObject>( Obase(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<MyTestObject>( Obase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCbase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjFac<Oderiv,  Fdflt>() ),
    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),

    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( Oderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyTestObject>( OCderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),


    // negative tests for deleters look for a null pointer lvalue.
    // Note that null pointer literal would be a compile-fail test
    TestPolicy<MyTestObject>( Obase(),   Ftst(), NullPolicy() ),
    TestPolicy<MyTestObject>( Obase(),   Fbsl(), NullPolicy() ),
    TestPolicy<MyTestObject>( Oderiv(),  Ftst(), NullPolicy() ),
    TestPolicy<MyTestObject>( Oderiv(),  Fbsl(), NullPolicy() ),
    //TestPolicy<MyTestObject>( OCbase(),  Ftst(), NullPolicy() ),
    //TestPolicy<MyTestObject>( OCbase(),  Fbsl(), NullPolicy() ),
    //TestPolicy<MyTestObject>( OCderiv(), Ftst(), NullPolicy() ),
    //TestPolicy<MyTestObject>( OCderiv(), Fbsl(), NullPolicy() )
};

//-----------------------------------------------------------------------------
// This is the test table for iterating constructor and load functions for
// 'bslma::ManagedPtr<MyTestObject>'.  The same test table is created for each
// of the main 5 tested pointer types, and then the invalid functions are
// commented out, to audit that they have intentionally been reviewed and
// rejected.  This allows us to compare the different test tables if a
// discrepancy occurs in the future.
static const TestPolicy<const MyTestObject> TEST_POLICY_CONST_BASE_ARRAY[] = {
    // default test
    TestPolicy<const MyTestObject>(),

    // single object-pointer tests
    TestPolicy<const MyTestObject>( NullPolicy() ),

    TestPolicy<const MyTestObject>( Obase() ),
    TestPolicy<const MyTestObject>( Oderiv() ),
    TestPolicy<const MyTestObject>( OCbase() ),
    TestPolicy<const MyTestObject>( OCderiv() ),

    // factory tests
    TestPolicy<const MyTestObject>( NullPolicy(), NullPolicy() ),

    TestPolicy<const MyTestObject>( NullPolicy(), Ftst() ),
    TestPolicy<const MyTestObject>( NullPolicy(), Fbsl() ),
    TestPolicy<const MyTestObject>( Obase(),      Ftst() ),
    TestPolicy<const MyTestObject>( Obase(),      Fbsl() ),
    TestPolicy<const MyTestObject>( Oderiv(),     Ftst() ),
    TestPolicy<const MyTestObject>( Oderiv(),     Fbsl() ),
    TestPolicy<const MyTestObject>( OCbase(),     Ftst() ),
    TestPolicy<const MyTestObject>( OCbase(),     Fbsl() ),
    TestPolicy<const MyTestObject>( OCderiv(),    Ftst() ),
    TestPolicy<const MyTestObject>( OCderiv(),    Fbsl() ),

    // deleter tests
    TestPolicy<const MyTestObject>( NullPolicy(), NullPolicy(), NullPolicy() ),

    // First test the non-deprecated interface, using the policy
    // 'DVoidVoid'.

    // MyTestObject
    TestPolicy<const MyTestObject>( Obase(), Ftst(), DVoidVoid< Obase,   Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyTestObject
    TestPolicy<const MyTestObject>( OCbase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( OCbase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCbase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const MyTestObject>( Obase(),  Fdflt(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),


    // Next we test the deprecated support for deleters other than
    // 'void (*)(void *, void *)', starting with deleters that
    // type-erase the 'object' type, but have a strongly typed
    // 'factory' argument.  Such deleters are generated by the
    // 'DVoidFac' policy..

    // MyTestObject
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DVoidFac< Obase,   Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyTestObject
    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( OCbase(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DVoidFac<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<Oderiv,  Fdflt>() ),
    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),

    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    // HERE WE ARE DOUBLY-BROKEN AS CV-QUALIFIED TYPES ARE NOT
    // SUPPORTED FOR TYPE-ERASURE THROUGH DELETER
    //TestPolicy<const MyTestObject>( OCbase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),


    // Now we test deleters that are strongly typed for the
    // 'object' parameter, but type-erase the 'factory'.

    // MyTestObject
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjVoid< Obase,   Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyTestObject
    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( OCbase(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),


    // Finally we test the most generic combination of generic
    // object type, a factory, and a deleter taking two arguments
    // compatible with pointers to the invoking 'object' and
    // 'factory' types.

    // MyTestObject
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjFac< Obase,   Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Obase(),   Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyTestObject
    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<const MyTestObject>( OCbase(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DObjFac<Obase,   Fdflt>() ),
    TestPolicy<const MyTestObject>( Obase(),   Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCbase(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjFac<Oderiv,  Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjFac<Obase,   Fdflt>() ),

    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<const MyTestObject>( Obase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const MyTestObject>( OCbase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjFac<Oderiv,  Fdflt>() ),
    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),

    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<const MyTestObject>( Oderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<const MyTestObject>( OCderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),


    // negative tests for deleters look for a null pointer lvalue.
    // Note that null pointer literal would be a compile-fail test
    TestPolicy<const MyTestObject>( Obase(),   Ftst(), NullPolicy() ),
    TestPolicy<const MyTestObject>( Obase(),   Fbsl(), NullPolicy() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Ftst(), NullPolicy() ),
    TestPolicy<const MyTestObject>( Oderiv(),  Fbsl(), NullPolicy() ),
    TestPolicy<const MyTestObject>( OCbase(),  Ftst(), NullPolicy() ),
    TestPolicy<const MyTestObject>( OCbase(),  Fbsl(), NullPolicy() ),
    TestPolicy<const MyTestObject>( OCderiv(), Ftst(), NullPolicy() ),
    TestPolicy<const MyTestObject>( OCderiv(), Fbsl(), NullPolicy() )
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This is the test table for iterating constructor and load functions for
// 'bslma::ManagedPtr<MyTestObject>'.  The same test table is created for each
// of the main 5 tested pointer types, and then the invalid functions are
// commented out, to audit that they have intentionally been reviewed and
// rejected.  This allows us to compare the different test tables if a
// discrepancy occurs in the future.
// In particular, this case does not support construction from pointers to
// 'const' objects, or from pointers to base objects.
static const TestPolicy<MyDerivedObject> TEST_POLICY_DERIVED_ARRAY[] = {
    // default test
    TestPolicy<MyDerivedObject>(),

    // single object-pointer tests
    TestPolicy<MyDerivedObject>( NullPolicy() ),
    TestPolicy<MyDerivedObject>( NullPolicy(), NullPolicy(), NullPolicy() ),
    TestPolicy<MyDerivedObject>( NullPolicy(), NullPolicy() ),

    //TestPolicy<MyDerivedObject>( Obase() ),
    TestPolicy<MyDerivedObject>( Oderiv() ),
    //TestPolicy<MyDerivedObject>( OCbase() ),
    //TestPolicy<MyDerivedObject>( OCderiv() ),

    // factory tests

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Fbsl() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl() ),
    // deleter tests

    // First test the non-deprecated interface, using the policy
    // 'DVoidVoid'.

    // MyDerivedObject
    //TestPolicy<MyDerivedObject>( Obase(), Ftst(), DVoidVoid< Obase,   Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    //TestPolicy<MyDerivedObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCbase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCbase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCbase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Fbsl(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    //TestPolicy<MyDerivedObject>( Obase(),  Fdflt(), DVoidVoid<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Fdflt(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Fdflt(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),


    // Next we test the deprecated support for deleters other than
    // 'void (*)(void *, void *)', starting with deleters that
    // type-erase the 'object' type, but have a strongly typed
    // 'factory' argument.  Such deleters are generated by the
    // 'DVoidFac' policy..

    // MyDerivedObject
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DVoidFac< Obase,   Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Ftst >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DVoidFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DVoidFac<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DVoidFac<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidFac<Oderiv,  Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),

    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    // HERE WE ARE DOUBLY-BROKEN AS CV-QUALIFIED TYPES ARE NOT
    // SUPPORTED FOR TYPE-ERASURE THROUGH DELETER
    //TestPolicy<MyDerivedObject>( OCbase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),


    // Now we test deleters that are strongly typed for the
    // 'object' parameter, but type-erase the 'factory'.

    // MyDerivedObject
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjVoid< Obase,   Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Ftst >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DObjVoid<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),


    // Finally we test the most generic combination of generic
    // object type, a factory, and a deleter taking two arguments
    // compatible with pointers to the invoking 'object' and
    // 'factory' types.

    // MyDerivedObject
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjFac< Obase,   Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DObjFac< Obase,   Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjFac< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Ftst >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Ftst >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), DObjFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjFac<Oderiv,  Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjFac<Obase,   Fdflt>() ),

    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Obase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCbase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjFac<Oderiv,  Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),

    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( Oderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),


    // negative tests for deleters look for a null pointer lvalue.
    // Note that null pointer literal would be a compile-fail test
    //TestPolicy<MyDerivedObject>( Obase(),   Ftst(), NullPolicy() ),
    //TestPolicy<MyDerivedObject>( Obase(),   Fbsl(), NullPolicy() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Ftst(), NullPolicy() ),
    TestPolicy<MyDerivedObject>( Oderiv(),  Fbsl(), NullPolicy() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Ftst(), NullPolicy() ),
    //TestPolicy<MyDerivedObject>( OCbase(),  Fbsl(), NullPolicy() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Ftst(), NullPolicy() ),
    //TestPolicy<MyDerivedObject>( OCderiv(), Fbsl(), NullPolicy() )
};

//=============================================================================
// This is the test table for iterating constructor and load functions for
// 'bslma::ManagedPtr<MyTestObject>'.  The same test table is created for each
// of the main 5 tested pointer types, and then the invalid functions are
// commented out, to audit that they have intentionally been reviewed and
// rejected.  This allows us to compare the different test tables if a
// discrepancy occurs in the future.
// In particular, this case does not support construction from pointers to
// 'const' objects.
static const TestPolicy<void> TEST_POLICY_VOID_ARRAY[] = {
    // default test
    TestPolicy<void>(),

    // single object-pointer tests
    TestPolicy<void>( NullPolicy() ),


    TestPolicy<void>( Obase() ),
    TestPolicy<void>( Oderiv() ),
    //TestPolicy<void>( OCbase() ),
    //TestPolicy<void>( OCderiv() ),

    TestPolicy<void>( Ob1() ),
    TestPolicy<void>( Ob2() ),
    TestPolicy<void>( Ocomp() ),

    // factory tests
    TestPolicy<void>( NullPolicy(), NullPolicy() ),

    TestPolicy<void>( Obase(),   Ftst() ),
    TestPolicy<void>( Obase(),   Fbsl() ),
    TestPolicy<void>( Oderiv(),  Ftst() ),
    TestPolicy<void>( Oderiv(),  Fbsl() ),
    //TestPolicy<void>( OCbase(),  Ftst() ),
    //TestPolicy<void>( OCbase(),  Fbsl() ),
    //TestPolicy<void>( OCderiv(), Ftst() ),
    //TestPolicy<void>( OCderiv(), Fbsl() ),


    TestPolicy<void>( Ob1(),   Ftst() ),
    TestPolicy<void>( Ob1(),   Fbsl() ),
    TestPolicy<void>( Ob2(),   Ftst() ),
    TestPolicy<void>( Ob2(),   Fbsl() ),
    TestPolicy<void>( Ocomp(), Ftst() ),
    TestPolicy<void>( Ocomp(), Fbsl() ),

    TestPolicy<void>( Ocomp(), Ftst(), DVoidVoid< Ocomp, Ftst >() ),
    TestPolicy<void>( Ocomp(), Fbsl(), DVoidVoid< Ocomp, Fbsl >() ),

    TestPolicy<void>( Ocomp(), Ftst(), DObjFac< Ocomp,   Ftst >() ),
    TestPolicy<void>( Ocomp(), Fbsl(), DObjFac< Ocomp,   Fbsl >() ),

    // deleter tests
    TestPolicy<void>( NullPolicy(), NullPolicy(), NullPolicy() ),

    // First test the non-deprecated interface, using the policy
    // 'DVoidVoid'.

    // void
    TestPolicy<void>( Obase(), Ftst(), DVoidVoid< Obase,   Ftst >() ),
    TestPolicy<void>( Obase(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<void>( Obase(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Obase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<void>( Obase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<void>( Obase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const void
    //TestPolicy<void>( OCbase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    //TestPolicy<void>( OCbase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCbase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Ftst >() ),
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< Obase,   Ftst >() ),

    TestPolicy<void>( Oderiv(), Fbsl(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<void>( Oderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    //TestPolicy<void>( OCderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<void>( Obase(),  Fdflt(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(), Fdflt(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(), Fdflt(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<void>( Obase(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(), NullPolicy(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),


    // Next we test the deprecated support for deleters other than
    // 'void (*)(void *, void *)', starting with deleters that
    // type-erase the 'object' type, but have a strongly typed
    // 'factory' argument.  Such deleters are generated by the
    // 'DVoidFac' policy..

    // void
    TestPolicy<void>( Obase(),   Ftst(), DVoidFac< Obase,   Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Obase(),   Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const void
    //TestPolicy<void>( OCbase(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),
    //TestPolicy<void>( OCbase(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCbase(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< Obase,   Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidFac< OCderiv, Ftst >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidFac< OCbase,  Ftst >() ),

    //TestPolicy<void>( OCderiv(), Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCderiv(), Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DVoidFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<void>( Obase(),   Fdflt(), DVoidFac<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(),   Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DVoidFac<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DVoidFac<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<void>( Obase(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<void>( Obase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( Oderiv(), NullPolicy(), DVoidFac<Oderiv,  Fdflt>() ),
    //TestPolicy<void>( Oderiv(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),

    //TestPolicy<void>( Oderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( Oderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    // HERE WE ARE DOUBLY-BROKEN AS CV-QUALIFIED TYPES ARE NOT
    // SUPPORTED FOR TYPE-ERASURE THROUGH DELETER
    //TestPolicy<void>( OCbase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),


    // Now we test deleters that are strongly typed for the
    // 'object' parameter, but type-erase the 'factory'.

    // void
    TestPolicy<void>( Obase(),   Ftst(), DObjVoid< Obase,   Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Obase(),   Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const void
    //TestPolicy<void>( OCbase(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),
    //TestPolicy<void>( OCbase(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCbase(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< Obase,   Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<void>( OCderiv(), Ftst(), DObjVoid< OCderiv, Ftst >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DObjVoid< OCbase,  Ftst >() ),

    //TestPolicy<void>( OCderiv(), Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCderiv(), Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DObjVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<void>( Obase(),   Fdflt(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(),   Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<void>( Obase(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(), NullPolicy(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),


    // Finally we test the most generic combination of generic
    // object type, a factory, and a deleter taking two arguments
    // compatible with pointers to the invoking 'object' and
    // 'factory' types.

    // void
    TestPolicy<void>( Obase(),   Ftst(), DObjFac< Obase,   Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Obase(),   Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<void>( Obase(),   Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<void>( Obase(),   Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const void
    //TestPolicy<void>( OCbase(),  Ftst(), DObjFac< OCbase,  Ftst >() ),
    //TestPolicy<void>( OCbase(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCbase(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< Obase,   Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<void>( Oderiv(),  Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<void>( Oderiv(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    //TestPolicy<void>( OCderiv(), Ftst(), DObjFac< OCderiv, Ftst >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DObjFac< OCbase,  Ftst >() ),

    //TestPolicy<void>( OCderiv(), Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    //TestPolicy<void>( OCderiv(), Ftst(), DObjFac< OCderiv, Fbsl >() ),
    //TestPolicy<void>( OCderiv(), Ftst(), DObjFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<void>( Obase(),   Fdflt(), DObjFac<Obase,   Fdflt>() ),
    TestPolicy<void>( Obase(),   Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DObjFac<Oderiv,  Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DObjFac<Obase,   Fdflt>() ),

    TestPolicy<void>( Oderiv(),  Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<void>( Oderiv(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<void>( Obase(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<void>( Obase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCbase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( Oderiv(), NullPolicy(), DObjFac<Oderiv,  Fdflt>() ),
    //TestPolicy<void>( Oderiv(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),

    //TestPolicy<void>( Oderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( Oderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<void>( OCderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<void>( OCderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),


    // negative tests for deleters look for a null pointer lvalue.
    // Note that null pointer literal would be a compile-fail test
    TestPolicy<void>( Obase(),   Ftst(), NullPolicy() ),
    TestPolicy<void>( Obase(),   Fbsl(), NullPolicy() ),
    TestPolicy<void>( Oderiv(),  Ftst(), NullPolicy() ),
    TestPolicy<void>( Oderiv(),  Fbsl(), NullPolicy() ),
    //TestPolicy<void>( OCbase(),  Ftst(), NullPolicy() ),
    //TestPolicy<void>( OCbase(),  Fbsl(), NullPolicy() ),
    //TestPolicy<void>( OCderiv(), Ftst(), NullPolicy() ),
    //TestPolicy<void>( OCderiv(), Fbsl(), NullPolicy() )
};

//-----------------------------------------------------------------------------
// This is the test table for iterating constructor and load functions for
// 'bslma::ManagedPtr<MyTestObject>'.  The same test table is created for each
// of the main 5 tested pointer types, and then the invalid functions are
// commented out, to audit that they have intentionally been reviewed and
// rejected.  This allows us to compare the different test tables if a
// discrepancy occurs in the future.
static const TestPolicy<const void> TEST_POLICY_CONST_VOID_ARRAY[] = {
    // default test
    TestPolicy<const void>(),

    // single object-pointer tests
    TestPolicy<const void>( NullPolicy() ),

    TestPolicy<const void>( Obase() ),
    TestPolicy<const void>( Oderiv() ),
    TestPolicy<const void>( OCbase() ),
    TestPolicy<const void>( OCderiv() ),

    TestPolicy<const void>( Ob1() ),
    TestPolicy<const void>( Ob2() ),
    TestPolicy<const void>( Ocomp() ),

    // factory tests
    TestPolicy<const void>( NullPolicy(), NullPolicy() ),

    TestPolicy<const void>( Obase(),   Ftst() ),
    TestPolicy<const void>( Obase(),   Fbsl() ),
    TestPolicy<const void>( Oderiv(),  Ftst() ),
    TestPolicy<const void>( Oderiv(),  Fbsl() ),
    TestPolicy<const void>( OCbase(),  Ftst() ),
    TestPolicy<const void>( OCbase(),  Fbsl() ),
    TestPolicy<const void>( OCderiv(), Ftst() ),
    TestPolicy<const void>( OCderiv(), Fbsl() ),

    TestPolicy<const void>( Ob1(),     Ftst() ),
    TestPolicy<const void>( Ob1(),     Fbsl() ),
    TestPolicy<const void>( Ob2(),     Ftst() ),
    TestPolicy<const void>( Ob2(),     Fbsl() ),
    TestPolicy<const void>( Ocomp(),   Ftst() ),
    TestPolicy<const void>( Ocomp(),   Fbsl() ),

    TestPolicy<const void>( Ocomp(), Ftst(), DVoidVoid< Ocomp,   Ftst >() ),
    TestPolicy<const void>( Ocomp(), Fbsl(), DVoidVoid< Ocomp,   Fbsl >() ),
    TestPolicy<const void>( Ocomp(), Ftst(), DObjFac< Ocomp,   Ftst >() ),
    TestPolicy<const void>( Ocomp(), Fbsl(), DObjFac< Ocomp,   Fbsl >() ),

#if 0
    TestPolicy<const void>( Ocomp(), Ftst(), DVoidVoid< Ob1,     Fbsl >() ),
    TestPolicy<const void>( Ocomp(), Ftst(), DVoidVoid< Ob2,     Fbsl >() ),
    TestPolicy<const void>( Ocomp(), Ftst(), DObjFac< Ob1,     Fbsl >() ),
    TestPolicy<const void>( Ocomp(), Ftst(), DObjFac< Ob2,     Fbsl >() ),
#endif

    // deleter tests
    TestPolicy<const void>( NullPolicy(), NullPolicy(), NullPolicy() ),

    // First test the non-deprecated interface, using the policy
    // 'DVoidVoid'.

    // void
    TestPolicy<const void>( Obase(), Ftst(), DVoidVoid< Obase,   Ftst >() ),
    TestPolicy<const void>( Obase(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<const void>( Obase(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Obase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<const void>( Obase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Obase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const void
    TestPolicy<const void>( OCbase(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),
    TestPolicy<const void>( OCbase(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCbase(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Ftst >() ),
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< Obase,   Ftst >() ),

    TestPolicy<const void>( Oderiv(), Fbsl(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(), Fbsl(), DVoidVoid< Obase,   Fbsl >() ),

    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<const void>( Oderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Ftst >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Ftst >() ),

    TestPolicy<const void>( OCderiv(), Fbsl(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Fbsl(), DVoidVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCderiv(), Ftst(), DVoidVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const void>( Obase(),  Fdflt(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(),  Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(), Fdflt(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(), Fdflt(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), Fdflt(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), Fdflt(), DVoidVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<const void>( Obase(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidVoid<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), NullPolicy(), DVoidVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), NullPolicy(), DVoidVoid<OCbase,  Fdflt>() ),


    // Next we test the deprecated support for deleters other than
    // 'void (*)(void *, void *)', starting with deleters that
    // type-erase the 'object' type, but have a strongly typed
    // 'factory' argument.  Such deleters are generated by the
    // 'DVoidFac' policy..

    // void
    TestPolicy<const void>( Obase(),   Ftst(), DVoidFac< Obase,   Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Obase(),   Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const void
    TestPolicy<const void>( OCbase(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),
    TestPolicy<const void>( OCbase(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCbase(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< Obase,   Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DVoidFac< Obase,   Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DVoidFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidFac< OCderiv, Ftst >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidFac< OCbase,  Ftst >() ),

    TestPolicy<const void>( OCderiv(), Fbsl(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Fbsl(), DVoidFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCderiv(), Ftst(), DVoidFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DVoidFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const void>( Obase(),   Fdflt(), DVoidFac<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(),   Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DVoidFac<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DVoidFac<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), Fdflt(), DVoidFac<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), Fdflt(), DVoidFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<const void>( Obase(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),
    //TestPolicy<const void>( Obase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidFac<Oderiv,  Fdflt>() ),
    //TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidFac<Obase,   Fdflt>() ),

    //TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<const void>( Oderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    // HERE WE ARE DOUBLY-BROKEN AS CV-QUALIFIED TYPES ARE NOT
    // SUPPORTED FOR TYPE-ERASURE THROUGH DELETER
    //TestPolicy<const void>( OCbase(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),

    //TestPolicy<const void>( OCderiv(), NullPolicy(), DVoidFac<OCderiv, Fdflt>() ),
    //TestPolicy<const void>( OCderiv(), NullPolicy(), DVoidFac<OCbase,  Fdflt>() ),


    // Now we test deleters that are strongly typed for the
    // 'object' parameter, but type-erase the 'factory'.

    // void
    TestPolicy<const void>( Obase(),   Ftst(), DObjVoid< Obase,   Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Obase(),   Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const void
    TestPolicy<const void>( OCbase(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),
    TestPolicy<const void>( OCbase(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCbase(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< Obase,   Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjVoid< Obase,   Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjVoid< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const void>( OCderiv(), Ftst(), DObjVoid< OCderiv, Ftst >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DObjVoid< OCbase,  Ftst >() ),

    TestPolicy<const void>( OCderiv(), Fbsl(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Fbsl(), DObjVoid< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCderiv(), Ftst(), DObjVoid< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DObjVoid< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const void>( Obase(),   Fdflt(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(),   Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), Fdflt(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), Fdflt(), DObjVoid<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    TestPolicy<const void>( Obase(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(), NullPolicy(), DObjVoid<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(), NullPolicy(), DObjVoid<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), NullPolicy(), DObjVoid<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), NullPolicy(), DObjVoid<OCbase,  Fdflt>() ),


    // Finally we test the most generic combination of generic
    // object type, a factory, and a deleter taking two arguments
    // compatible with pointers to the invoking 'object' and
    // 'factory' types.

    // void
    TestPolicy<const void>( Obase(),   Ftst(), DObjFac< Obase,   Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Obase(),   Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<const void>( Obase(),   Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Obase(),   Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const void
    TestPolicy<const void>( OCbase(),  Ftst(), DObjFac< OCbase,  Ftst >() ),
    TestPolicy<const void>( OCbase(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCbase(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // MyDerivedObject
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< Obase,   Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjFac< Obase,   Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< Oderiv,  Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< Obase,   Fbsl >() ),

    // ... plus safe const-conversions
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), DObjFac< OCbase,  Fbsl >() ),

    // const MyDerivedObject
    TestPolicy<const void>( OCderiv(), Ftst(), DObjFac< OCderiv, Ftst >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DObjFac< OCbase,  Ftst >() ),

    TestPolicy<const void>( OCderiv(), Fbsl(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Fbsl(), DObjFac< OCbase,  Fbsl >() ),

    TestPolicy<const void>( OCderiv(), Ftst(), DObjFac< OCderiv, Fbsl >() ),
    TestPolicy<const void>( OCderiv(), Ftst(), DObjFac< OCbase,  Fbsl >() ),


    // Also test a deleter that does not use the 'factory'
    // argument.  These tests must also validate passing a null
    // pointer lvalue as the 'factory' argument.
    TestPolicy<const void>( Obase(),   Fdflt(), DObjFac<Obase,   Fdflt>() ),
    TestPolicy<const void>( Obase(),   Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCbase(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjFac<Oderiv,  Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjFac<Obase,   Fdflt>() ),

    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<const void>( Oderiv(),  Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    TestPolicy<const void>( OCderiv(), Fdflt(), DObjFac<OCderiv, Fdflt>() ),
    TestPolicy<const void>( OCderiv(), Fdflt(), DObjFac<OCbase,  Fdflt>() ),

    // Also, verify null pointer literal can be used for the
    // factory argument in each case.
    // DESIGN NOTE - NULL POINTER LITERALS CAN BE USED ONLY WITH
    //               DELETERS THAT TYPE-ERASE THE FACTORY.
    //TestPolicy<const void>( Obase(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),
    //TestPolicy<const void>( Obase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const void>( OCbase(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const void>( Oderiv(), NullPolicy(), DObjFac<Oderiv,  Fdflt>() ),
    //TestPolicy<const void>( Oderiv(), NullPolicy(), DObjFac<Obase,   Fdflt>() ),

    //TestPolicy<const void>( Oderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<const void>( Oderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),

    //TestPolicy<const void>( OCderiv(), NullPolicy(), DObjFac<OCderiv, Fdflt>() ),
    //TestPolicy<const void>( OCderiv(), NullPolicy(), DObjFac<OCbase,  Fdflt>() ),


    // negative tests for deleters look for a null pointer lvalue.
    // Note that null pointer literal would be a compile-fail test
    TestPolicy<const void>( Obase(),   Ftst(), NullPolicy() ),
    TestPolicy<const void>( Obase(),   Fbsl(), NullPolicy() ),
    TestPolicy<const void>( Oderiv(),  Ftst(), NullPolicy() ),
    TestPolicy<const void>( Oderiv(),  Fbsl(), NullPolicy() ),
    TestPolicy<const void>( OCbase(),  Ftst(), NullPolicy() ),
    TestPolicy<const void>( OCbase(),  Fbsl(), NullPolicy() ),
    TestPolicy<const void>( OCderiv(), Ftst(), NullPolicy() ),
    TestPolicy<const void>( OCderiv(), Fbsl(), NullPolicy() )
};

//=============================================================================
// Here we add additional test cases for the deliberately awkward 'composite'
// case, which does not use a virtual destructor.  Note that we cannot test
// this on the couple of platforms where we get the wrong answer from the
// 'bslmf::IsPolymorphic' type trait, as the workaround makes the 'Base' class
// ambiguous.
#if defined(BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE)
static const TestPolicy<Base> TEST_POLICY_BASE0_ARRAY[] = {
    // default test
    TestPolicy<Base>(),

    // single object-pointer tests
    TestPolicy<Base>( NullPolicy() ),

    TestPolicy<Base>( Ob1() ),
    TestPolicy<Base>( Ob2() ),
    TestPolicy<Base>( Ocomp() ),

    // factory tests
    TestPolicy<Base>( NullPolicy(), NullPolicy() ),

    TestPolicy<Base>( Ob1(),   Ftst() ),
    TestPolicy<Base>( Ob1(),   Fbsl() ),
    TestPolicy<Base>( Ob2(),   Ftst() ),
    TestPolicy<Base>( Ob2(),   Fbsl() ),
    TestPolicy<Base>( Ocomp(), Ftst() ),
    TestPolicy<Base>( Ocomp(), Fbsl() ),

    // deleter tests
    TestPolicy<Base>( Ocomp(), Ftst(), DVoidVoid< Ocomp,   Ftst >() ),
    TestPolicy<Base>( Ocomp(), Fbsl(), DVoidVoid< Ocomp,   Fbsl >() ),
    TestPolicy<Base>( Ocomp(), Ftst(), DObjFac< Ocomp,   Ftst >() ),
    TestPolicy<Base>( Ocomp(), Fbsl(), DObjFac< Ocomp,   Fbsl >() ),
};
#endif

// This is the important test case for composites, where 'Base2' is the
// *second* base class from 'Composite', which uses multiple inheritance.  Note
// that this test is equally important for platforms where
// 'bslmf::IsPolymorphic' is giving the wrong answer - that is why we have a
// second workaround version of 'Composite' available for this test.
static const TestPolicy<Base2> TEST_POLICY_BASE2_ARRAY[] = {
    // default test
    TestPolicy<Base2>(),

    // single object-pointer tests
    TestPolicy<Base2>( NullPolicy() ),

    TestPolicy<Base2>( Ob2() ),
    TestPolicy<Base2>( Ocomp() ),

    // factory tests
    TestPolicy<Base2>( NullPolicy(), NullPolicy() ),

    TestPolicy<Base2>( Ob2(),   Ftst() ),
    TestPolicy<Base2>( Ob2(),   Fbsl() ),
    TestPolicy<Base2>( Ocomp(), Ftst() ),
    TestPolicy<Base2>( Ocomp(), Fbsl() ),

    // deleter tests
    TestPolicy<Base2>( Ocomp(), Ftst(), DVoidVoid< Ocomp,   Ftst >() ),
    TestPolicy<Base2>( Ocomp(), Fbsl(), DVoidVoid< Ocomp,   Fbsl >() ),
    TestPolicy<Base2>( Ocomp(), Ftst(), DObjFac< Ocomp,   Ftst >() ),
    TestPolicy<Base2>( Ocomp(), Fbsl(), DObjFac< Ocomp,   Fbsl >() ),
};

}  // close unnamed namespace

//=============================================================================
//                                USAGE EXAMPLE
//-----------------------------------------------------------------------------
namespace USAGE_EXAMPLES {

///Example 1: Implementing a protocol
/// - - - - - - - - - - - - - - - - -
// We demonstrate using 'bslma::ManagedPtr' to configure and return a managed
// object implementing an abstract protocol.
//
// First we define our protocol, 'Shape', a type of object that knows how to
// compute its 'area'.  Note that for expository reasons only, we do *nor*
// give 'Shape' a virtual destructor.
//..
    struct Shape {
        virtual double area() const = 0;
            // Return the 'area' of this shape.
    };
//..
// Then we define a couple of classes that implement the 'Shape' protocol, a
// 'Circle' and a 'Square'.
//..
    class Circle : public Shape {
      private:
        // DATA
        double d_radius;

      public:
        // CREATORS
        explicit Circle(double r);
            // Create a 'Circle' object having radius 'r'.

        // ACCESSORS
        virtual double area() const;
            // Return the area of this Circle, given by the formula pi*r*r.
    };

    class Square : public Shape {
      private:
        // DATA
        double d_sideLength;

      public:
        // CREATORS
        explicit Square(double side);
            // Create a 'Square' having sides of length 'side'.

        // ACCESSORS
        virtual double area() const;
            // Return the area of this Square, given by the formula side*side
    };
//..
// Next we implement the methods for 'Circle' and 'Square'.
//..
    Circle::Circle(double r)
    : d_radius(r)
    {
    }

    double Circle::area() const
    {
        return 3.141592653589793238462 * d_radius * d_radius;
    }

    Square::Square(double side)
    : d_sideLength(side)
    {
    }

    double Square::area() const
    {
        return d_sideLength * d_sideLength;
    }
//..
// Then we define an enumeration that lists each implementation of the 'Shape'
// protocol.
//..
    struct Shapes {
        enum VALUES { SHAPE_CIRCLE, SHAPE_SQUARE };
    };
//..
// Now we can define a function that will return a 'Circle' object or a
// 'Square' object according to the specified 'kind' parameter, and having its
// 'dimension' specified by the caller.
//..
    bslma::ManagedPtr<Shape> makeShape(Shapes::VALUES kind, double dimension)
    {
        bslma::Allocator *alloc = bslma::Default::defaultAllocator();
        bslma::ManagedPtr<Shape> result;
        switch (kind) {
            case Shapes::SHAPE_CIRCLE : {
                Circle *circ = new(*alloc)Circle(dimension);
                result.load(circ);
                break;
            }
            case Shapes::SHAPE_SQUARE : {
                Square *sqr = new(*alloc)Square(dimension);
                result.load(sqr);
                break;
            }
        };
        return result;
    }
//..
// Then, we can use our function to create shapes of different kinds, and check
// that they report the correct area.  Note that are using a radius of '1.0'
// for the 'Circle' and integral side-length for the 'Square' to support an
// accurate 'operator==' with floating-point quantities.  Also note that,
// despite the destructor for 'Shape' being non-virtual, the correct destructor
// for the appropriate concrete 'Shape' type is called.  This is because the
// destructor is captured when the 'bslma::ManagedPtr' constructor is called,
// and has access to the complete type of each shape object.
//..
    void testShapes()
    {
        bslma::ManagedPtr<Shape> shape = makeShape(Shapes::SHAPE_CIRCLE, 1.0);
        ASSERT(0 != shape);
        ASSERT(3.141592653589793238462 == shape->area());

        shape = makeShape(Shapes::SHAPE_SQUARE, 2.0);
        ASSERT(0 != shape);
        ASSERT(4.0 == shape->area());
    }
//..
// Next, we observe that as we are creating objects dynamically, we should pass
// an allocator to the 'makeShape' function, rather than simply accepting the
// default allocator each time.  Note that when we do this, we pass the user's
// allocator to the 'bslma::ManagedPtr' object as the "factory".
//..
    bslma::ManagedPtr<Shape> makeShape(Shapes::VALUES    kind,
                                      double            dimension,
                                      bslma::Allocator *allocator)
    {
        bslma::Allocator *alloc = bslma::Default::allocator(allocator);
        bslma::ManagedPtr<Shape> result;
        switch (kind) {
        case Shapes::SHAPE_CIRCLE : {
                Circle *circ = new(*alloc)Circle(dimension);
                result.load(circ, alloc);
                break;
            }
        case Shapes::SHAPE_SQUARE : {
                Square *sqr = new(*alloc)Square(dimension);
                result.load(sqr, alloc);
                break;
            }
        };
        return result;
    }
//..
// Finally we repeat the earlier test, additionally passing a test allocator:
//..
    void testShapesToo()
    {
        bslma::TestAllocator ta("object");

        bslma::ManagedPtr<Shape> shape =
                                     makeShape(Shapes::SHAPE_CIRCLE, 1.0, &ta);
        ASSERT(0 != shape);
        ASSERT(3.141592653589793238462 == shape->area());

        shape = makeShape(Shapes::SHAPE_SQUARE, 3.0, &ta);
        ASSERT(0 != shape);
        ASSERT(9.0 == shape->area());
    }
//..
//
///Example 2: Aliasing
///- - - - - - - - - -
// Suppose that we wish to give access to an item in a temporary array via a
// pointer which we'll call the "finger".  The finger is the only pointer to
// the array or any part of the array, but the entire array must be valid until
// the finger is destroyed, at which time the entire array must be deleted.  We
// handle this situation by first creating a managed pointer to the entire
// array, then creating an alias of that pointer for the finger.  The finger
// takes ownership of the array instance, and when the finger is destroyed, it
// is the array's address, rather than the finger, that is passed to the
// deleter.
//
// First, let's say our array stores data acquired from a ticker plant
// accessible by a global 'getQuote' function:
//..
    struct Ticker {

        static double getQuote() // From ticker plant. Simulated here
        {
            static const double QUOTES[] = {
            7.25, 12.25, 11.40, 12.00, 15.50, 16.25, 18.75, 20.25, 19.25, 21.00
            };
            static const int NUM_QUOTES = sizeof(QUOTES) / sizeof(QUOTES[0]);
            static int index = 0;

            double ret = QUOTES[index];
            index = (index + 1) % NUM_QUOTES;
            return ret;
        }
    };
//..
// Then, we want to find the first quote larger than a specified threshold, but
// would also like to keep the earlier and later quotes for possible
// examination.  Our 'getFirstQuoteLargerThan' function must allocate memory
// for an array of quotes (the threshold and its neighbors).  It thus returns a
// managed pointer to the desired value:
//..
    const double END_QUOTE = -1;

    bslma::ManagedPtr<double>
    getFirstQuoteLargerThan(double threshold, bslma::Allocator *allocator)
    {
        ASSERT( END_QUOTE < 0 && 0 <= threshold );
//..
// Next, we allocate our array with extra room to mark the beginning and end
// with a special 'END_QUOTE' value:
//..
        const int MAX_QUOTES = 100;
        int numBytes = (MAX_QUOTES + 2) * sizeof(double);
        double *quotes = (double*) allocator->allocate(numBytes);
        quotes[0] = quotes[MAX_QUOTES + 1] = END_QUOTE;
//..
// Then, we create a managed pointer to the entire array:
//..
        bslma::ManagedPtr<double> managedQuotes(quotes, allocator);
//..
// Next, we read quotes until the array is full, keeping track of the first
// quote that exceeds the threshold.
//..
        double *finger = 0;

        for (int i = 1; i <= MAX_QUOTES; ++i) {
            double quote = Ticker::getQuote();
            quotes[i] = quote;
            if (!finger && quote > threshold) {
                finger = &quotes[i];
            }
        }
//..
// Now, we use the alias constructor to create a managed pointer that points to
// the desired value (the finger) but manages the entire array:
//..
        return bslma::ManagedPtr<double>(managedQuotes, finger);
    }
//..
// Then, our main program calls 'getFirstQuoteLargerThan' like this:
//..
    int aliasExample()
    {
        bslma::TestAllocator ta;
        bslma::ManagedPtr<double> result = getFirstQuoteLargerThan(16.00, &ta);
        ASSERT(*result > 16.00);
        ASSERT(1 == ta.numBlocksInUse());
        if (g_verbose) printf("Found quote: %g\n", *result);
//..
// Next, We also print the preceding 5 quotes in last-to-first order:
//..
        if (g_verbose) printf("Preceded by:");
        int i;
        for (i = -1; i >= -5; --i) {
            double quote = result.ptr()[i];
            if (END_QUOTE == quote) {
                break;
            }
            ASSERT(quote < *result);
            if (g_verbose) printf(" %g", quote);
        }
        if (g_verbose) printf("\n");
//..
// Then, to move the finger, e.g., to the last position printed, one must be
// careful to retain the ownership of the entire array.  Using the statement
// 'result.load(result.ptr()-i)' would be an error, because it would first
// compute the pointer value 'result.ptr()-i' of the argument, then release the
// entire array before starting to manage what has now become an invalid
// pointer.  Instead, 'result' must retain its ownership to the entire array,
// which can be attained by:
//..
        result.loadAlias(result, result.ptr()-i);
//..
// Finally, if we reset the result pointer, the entire array is deallocated:
//..
        result.clear();
        ASSERT(0 == ta.numBlocksInUse());
        ASSERT(0 == ta.numBytesInUse());

        return 0;
    }
//..
//
///Example 3: Dynamic Objects and Factories
/// - - - - - - - - - - - - - - - - - - - -
// Suppose we want to track the number of objects currently managed by
// 'bslma::ManagedPtr' objects.
//
// First we define a factory type, that holds an allocator and a usage-counter.
// Note that such a type cannot sensibly be copied, as the notion 'count'
// becomes confused.
//..
    class CountedFactory {
        // DATA
        int               d_count;
        bslma::Allocator *d_allocator;

      private:
        // NOT IMPLEMENTED
        CountedFactory(const CountedFactory&);
        CountedFactory& operator=(const CountedFactory&);

      public:
        // CREATORS
        explicit CountedFactory(bslma::Allocator *alloc = 0);
            // Create a 'CountedFactory' object which uses the supplied
            // allocator 'alloc'.

        ~CountedFactory();
            // Destroy this object.
//..
// Next, we provide the 'createObject' and 'deleteObject' functions that are
// standard for factory objects.  Note that the 'deleteObject' function
// signature has the form required by 'bslma::ManagedPtr' for a factory.
//..
        // MANIPULATORS
        template <class TYPE>
        TYPE *createObject();
            // Return a pointer to a newly allocated object of type 'TYPE'
            // created using its default constructor.  Memory for the object is
            // supplied by the allocator supplied to this factory's
            // constructor, and the count of valid object is incremented.

        template <class TYPE>
        void deleteObject(const TYPE *target);
            // Destroy the object pointed to be 'target' and reclaim the
            // memory.  Decrement the count of currently valid objects.
//..
// Then, we round out the class with the ability to query the 'count' of
// currently allocated objects.
//..
        // ACCESSORS
        int count() const;
            // Return the number of currently valid objects allocated by this
            // factory.
    };
//..
// Next, we define the operations declared by the class.
//..
    CountedFactory::CountedFactory(bslma::Allocator *alloc)
    : d_count(0)
    , d_allocator(bslma::Default::allocator(alloc))
    {
    }

    CountedFactory::~CountedFactory()
    {
        ASSERT(0 == d_count);
    }

    template <class TYPE>
    TYPE *CountedFactory::createObject()
    {
        TYPE *result = new(*d_allocator)TYPE;
        ++d_count;
        return result;
    }

    template <class TYPE>
    void CountedFactory::deleteObject(const TYPE *object)
    {
        d_allocator->deleteObject(object);
        --d_count;
    }

    inline
    int CountedFactory::count() const
    {
        return d_count;
    }
//..
// Then, we can create a test function to illustrate how such a factory would
// be used with 'bslma::ManagedPtr'.
//..
    void testCountedFactory()
    {
//..
// Next, we declare a test allocator, and an object of our 'CountedFactory'
// type using that allocator.
//..
        bslma::TestAllocator ta;
        CountedFactory cf(&ta);
//..
// Then, we open a new local scope and declare an array of managed pointers.
// We need a local scope in order to observe the behavior of the destructors at
// end of the scope, and use an array as an easy way to count more than one
// object.
//..
        {
            bslma::ManagedPtr<int> pData[4];
//..
// Next, we load each managed pointer in the array with a new 'int' using our
// factory 'cf' and assert that the factory 'count' is correct after each new
// 'int' is created.
//..
            int i = 0;
            while (i != 4) {
                pData[i++].load(cf.createObject<int>(), &cf);
                ASSERT(cf.count() == i);
            }
//..
// Then, we 'clear' the contents of a single managed pointer in the array, and
// assert that the factory 'count' is appropriately reduced.
//..
            pData[1].clear();
            ASSERT(3 == cf.count());
//..
// Next, we 'load' a managed pointer with another new 'int' value, again using
// 'cf' as the factory, and assert that the 'count' of valid objects remains
// the same (destroy one object and add another).
//..
            pData[2].load(cf.createObject<int>(), &cf);
            ASSERT(3 == cf.count());
        }
//..
// Finally, we allow the array of managed pointers to go out of scope and
// confirm that when all managed objects are destroyed, the factory 'count'
// falls to zero, and does not overshoot.
//..
        ASSERT(0 == cf.count());
    }
//..
}  // close namespace USAGE_EXAMPLES

//=============================================================================
//                                CASTING EXAMPLE
//-----------------------------------------------------------------------------
namespace TYPE_CASTING_TEST_NAMESPACE {

    typedef MyTestObject A;
    typedef MyDerivedObject B;

///Example 4: Type Casting
///- - - - - - - - - - - -
// 'bslma::ManagedPtr' objects can be implicitly and explicitly cast to
// different types in the same way that native pointers can.
//
///Implicit Conversion
/// -  -  -  -  -  - -
// As with native pointers, a pointer of the type 'B' that is publicly derived
// from the type 'A', can be directly assigned a 'bslma::ManagedPtr' of 'A'.
//
// First, consider the following code snippets:
//..
    void implicitCastingExample()
    {
//..
// If the statements:
//..
        bslma::TestAllocator localDefaultTa;
        bslma::TestAllocator localTa;

        bslma::DefaultAllocatorGuard guard(&localDefaultTa);

        int numdels = 0;

        {
            B *b_p = 0;
            A *a_p = b_p;
//..
// are legal expressions, then the statements
//..
            bslma::ManagedPtr<A> a_mp1;
            bslma::ManagedPtr<B> b_mp1;

            ASSERT(!a_mp1 && !b_mp1);

            a_mp1 = b_mp1;      // conversion assignment of nil ptr to nil
            ASSERT(!a_mp1 && !b_mp1);

#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
            B *b_p2 = new (localDefaultTa) B(&numdels);
#else
            B *b_p2 = new B(&numdels);
#endif
            bslma::ManagedPtr<B> b_mp2(b_p2);    // default allocator
            ASSERT(!a_mp1 && b_mp2);

            a_mp1 = b_mp2;      // conversion assignment of nonnil ptr to nil
            ASSERT(a_mp1 && !b_mp2);

            B *b_p3 = new (localTa) B(&numdels);
            bslma::ManagedPtr<B> b_mp3(b_p3, &localTa);
            ASSERT(a_mp1 && b_mp3);

            a_mp1 = b_mp3;      // conversion assignment of nonnil to nonnil
            ASSERT(a_mp1 && !b_mp3);

            a_mp1 = b_mp3;      // conversion assignment of nil to nonnil
            ASSERT(!a_mp1 && !b_mp3);

            // constructor conversion init with nil
            bslma::ManagedPtr<A> a_mp4(b_mp3, b_mp3.ptr());
            ASSERT(!a_mp4 && !b_mp3);

            // constructor conversion init with nonnil
            B *p_b5 = new (localTa) B(&numdels);
            bslma::ManagedPtr<B> b_mp5(p_b5, &localTa);
            bslma::ManagedPtr<A> a_mp5(b_mp5, b_mp5.ptr());
            ASSERT(a_mp5 && !b_mp5);
            ASSERT(a_mp5.ptr() == p_b5);

            // constructor conversion init with nonnil
            B *p_b6 = new (localTa) B(&numdels);
            bslma::ManagedPtr<B> b_mp6(p_b6, &localTa);
            bslma::ManagedPtr<A> a_mp6(b_mp6);
            ASSERT(a_mp6 && !b_mp6);
            ASSERT(a_mp6.ptr() == p_b6);

            struct S {
                int d_i[10];
            };

#if 0
            S *pS = new (localTa) S;
            bslma::ManagedPtr<S> s_mp1(pS, &localTa);

            for (int i = 0; 10 > i; ++i) {
                pS->d_i[i] = i;
            }

            bslma::ManagedPtr<int> i_mp1(s_mp1, s_mp1->d_i + 4);
            ASSERT(4 == *i_mp1);
#endif

            ASSERT(200 == numdels);
        }

        ASSERT(400 == numdels);
    } // implicitCastingExample()
//..
//
///Explicit Conversion
/// -  -  -  -  -  - -
// Through "aliasing", a managed pointer of any type can be explicitly
// converted to a managed pointer of any other type using any legal cast
// expression.  For example, to static-cast a managed pointer of type A to a
// shared pointer of type B, one can simply do the following:
//..
    void explicitCastingExample()
    {
        bslma::ManagedPtr<A> a_mp;
        bslma::ManagedPtr<B> b_mp1(a_mp, static_cast<B*>(a_mp.ptr()));
//..
// or even use the less safe "C"-style casts:
//..
        // bslma::ManagedPtr<A> a_mp;
        bslma::ManagedPtr<B> b_mp2(a_mp, (B*)(a_mp.ptr()));

    } // explicitCastingExample()
//..
// Note that when using dynamic cast, if the cast fails, the target managed
// pointer will be reset to an unset state, and the source will not be
// modified.  Consider for example the following snippet of code:
//..
    void processPolymorphicObject(bslma::ManagedPtr<A> aPtr,
                                  bool *castSucceeded)
    {
        bslma::ManagedPtr<B> bPtr(aPtr, dynamic_cast<B*>(aPtr.ptr()));
        if (bPtr) {
            ASSERT(!aPtr);
            *castSucceeded = true;
        }
        else {
            ASSERT(aPtr);
            *castSucceeded = false;
        }
    }
//..
// If the value of 'aPtr' can be dynamically cast to 'B*' then ownership is
// transferred to 'bPtr', otherwise 'aPtr' is to be modified.  As previously
// stated, the managed object will be destroyed correctly regardless of how it
// is cast.

}  // close namespace TYPE_CASTING_TEST_NAMESPACE

//=============================================================================
//                  DRQS 30670366
//-----------------------------------------------------------------------------
namespace DRQS_30670366_NAMESPACE {


void testDeleter(int *expectedCookieValue, void *cookie)
{
    ASSERT(expectedCookieValue == cookie);
}

}  // close namespace TYPE_CASTING_TEST_NAMESPACE

//=============================================================================
//                  TEST PROGRAM
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    bool             verbose = argc > 2;
    bool         veryVerbose = argc > 3;
    bool     veryVeryVerbose = argc > 4;
    bool veryVeryVeryVerbose = argc > 5;
                   g_verbose = verbose;
               g_veryVerbose = veryVerbose;
       g_veryVeryVeryVerbose = veryVeryVeryVerbose;

    printf("TEST " __FILE__ " CASE %d\n", test);

    bslma::TestAllocator globalAllocator("global", veryVeryVeryVerbose);
    bslma::Default::setGlobalAllocator(&globalAllocator);

    bslma::TestAllocator da("default", veryVeryVeryVerbose);
    bslma::Default::setDefaultAllocator(&da);

    switch (test) { case 0:
      case 21: {
        // --------------------------------------------------------------------
        // DRQS 30670366
        //
        // Concerns
        //   Suppling a cookie of type 'void *' and a deletion functor of type
        //   void deleter(DERIVED_TYPE *, void *) supplies the correct cookie
        //   to the deletion functor.
        //
        // Plan:
        //   Replicated the .
        //
        // Testing:
        //
        // --------------------------------------------------------------------
        using namespace DRQS_30670366_NAMESPACE;
        if (verbose) printf("\nDRQS 30670366"
                            "\n-------------\n");

        {
            int cookie = 100;
            bslma::ManagedPtr<int> test(&cookie,
                                       (void *)&cookie,
                                       &testDeleter);
        }
        {
            int cookie = 100;
            bslma::ManagedPtr<int> test;
            test.load(&cookie, (void *)&cookie, &testDeleter);
        }
      } break;
      case 20: {
        // --------------------------------------------------------------------
        // TESTING CONVERSION EXAMPLES
        //
        // Concerns
        //   Test casting of managed pointers, both when the pointer is null
        //   and when it is not.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        using namespace TYPE_CASTING_TEST_NAMESPACE;

        if (verbose) printf("\nTYPE CASTING EXAMPLE"
                            "\n--------------------\n");

        int numdels = 0;

        {
            implicitCastingExample();
            explicitCastingExample();

            bool castSucceeded;

            bslma::TestAllocator ta("object", veryVeryVeryVerbose);
            bslma::TestAllocatorMonitor tam(&ta);

            processPolymorphicObject(returnManagedPtr(&numdels, &ta),
                                    &castSucceeded);
            ASSERT(!castSucceeded);
            processPolymorphicObject(
                         bslma::ManagedPtr<A>(returnDerivedPtr(&numdels, &ta)),
                        &castSucceeded);
            ASSERT(castSucceeded);
            processPolymorphicObject(
                   bslma::ManagedPtr<A>(returnSecondDerivedPtr(&numdels, &ta)),
                  &castSucceeded);
            ASSERT(!castSucceeded);

            returnManagedPtr(&numdels, &ta);
            returnDerivedPtr(&numdels, &ta);
            returnSecondDerivedPtr(&numdels, &ta);
        }

        LOOP_ASSERT(numdels, 20202 == numdels);
      } break;
      case 19: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE 2
        //
        // Concerns
        //: 1 The usage example provided in the component header file must
        //:   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //: 1 Incorporate usage example from header into driver, remove leading
        //:   comment characters, and replace 'assert' with 'ASSERT'.  (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE 2
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING USAGE EXAMPLE 2"
                            "\n-----------------------\n");

        USAGE_EXAMPLES::aliasExample();

        // move usage-example 3 up to its own case
        USAGE_EXAMPLES::testCountedFactory();

      } break;
      case 18: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE 1
        //
        // Concerns
        //: 1 The usage example compiles and runs correctly.
        //
        // Plan:
        //: 1 Compile and run the usage example (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE 1
        // --------------------------------------------------------------------
        if (verbose) printf("\nTESTING Usage Example 1"
                            "\n-----------------------\n");

        USAGE_EXAMPLES::testShapes();
        USAGE_EXAMPLES::testShapesToo();
      } break;
      case 17: {
        // --------------------------------------------------------------------
        // TESTING bslma::ManagedPtrNilDeleter
        //
        // Concerns:
        //: 1 The 'deleter' method can be used as a deleter policy by
        //:   'bslma::ManagedPtr'.
        //:
        //: 2 When invoked, 'bslma::ManagedPtrNilDeleter<T>::deleter' has no
        //:   effect.
        //:
        //: 3 No memory is allocated from the global or default allocators.
        //
        // Plan:
        //: 1 blah ...
        //
        // Testing:
        //   bslma::ManagedPtrNilDeleter<T>::deleter
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bslma::ManagedPtrNilDeleter"
                            "\n----------------------------------\n");

        if (verbose) printf("\tConfirm the deleter does not destroy the "
                             "passsed object\n");

        int deleteCount = 0;
        MyTestObject t(&deleteCount);
        bslma::ManagedPtrNilDeleter<MyTestObject>::deleter(&t, 0);
        LOOP_ASSERT(deleteCount, 0 == deleteCount);

        if (verbose) printf("\tConfirm the deleter can be registered with "
                             "a managed pointer\n");

        bslma::TestAllocatorMonitor gam(&globalAllocator);
        bslma::TestAllocatorMonitor dam(&da);

        int x;
        int y;
        {
            bslma::ManagedPtr<int> p(
                                   &x,
                                   0,
                                   &bslma::ManagedPtrNilDeleter<int>::deleter);
            ASSERT(dam.isInUseSame());
            ASSERT(gam.isInUseSame());

            p.load(&y, 0, &bslma::ManagedPtrNilDeleter<int>::deleter);
            ASSERT(dam.isInUseSame());
            ASSERT(gam.isInUseSame());
        }
        ASSERT(dam.isInUseSame());
        ASSERT(gam.isInUseSame());
      } break;
      case 16: {
        // --------------------------------------------------------------------
        // TESTING bslma::ManagedPtrNoOpDeleter
        //
        // Concerns:
        //: 1 The 'deleter' method can be used as a deleter policy by
        //:   'bslma::ManagedPtr'.
        //:
        //: 2 When invoked, 'bslma::ManagedPtrNoOpDeleter::deleter' has no
        //:   effect.
        //:
        //: 3 No memory is allocated from the global or default allocators.
        //
        // Plan:
        //: 1 blah ...
        //
        // Testing:
        //    bslma::ManagedPtrNoOpDeleter::deleter
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bslma::ManagedPtrNoOpDeleter"
                            "\n-----------------------------------\n");

        if (verbose) printf("\tConfirm the deleter does not destroy the "
                            "passsed object\n");

        int deleteCount = 0;
        MyTestObject t(&deleteCount);
        bslma::ManagedPtrUtil::noOpDeleter(&t, 0);
        LOOP_ASSERT(deleteCount, 0 == deleteCount);

        if (verbose) printf("\tConfirm the deleter can be registered with "
                            "a managed pointer\n");

        bslma::TestAllocatorMonitor gam(&globalAllocator);
        bslma::TestAllocatorMonitor dam(&da);

        int x;
        int y;
        {
            bslma::ManagedPtr<int> p(&x,
                                     0,
                                     &bslma::ManagedPtrUtil::noOpDeleter);
            ASSERT(dam.isInUseSame());
            ASSERT(gam.isInUseSame());

            p.load(&y, 0, bslma::ManagedPtrUtil::noOpDeleter);
            ASSERT(dam.isInUseSame());
            ASSERT(gam.isInUseSame());
        }
        ASSERT(dam.isInUseSame());
        ASSERT(gam.isInUseSame());
      } break;
      case 15: {
        // --------------------------------------------------------------------
        // CLEAR and RELEASE
        //
        // Concerns:
        //: 1 'clear' destroys the managed object (if any) and re-initializes
        //:   the managed pointer to an unset state.
        //:
        //: 2 'clear' destroys any managed object using the stored 'deleter'.
        //:
        //   That release works properly.
        //   Release gives up ownership of resources without running deleters
        //
        //   Test each function behaves correctly given one of the following
        //     kinds of managed pointer objects:
        //     empty
        //     simple
        //     simple with factory
        //     simple with factory and deleter
        //     aliased
        //     aliased (original created with factory)
        //     aliased (original created with factory and deleter)
        //
        // Plan:
        //   TBD...
        //
        // Tested:
        //   void clear();
        //   bsl::pair<TYPE*, bslma::ManagedPtrDeleter> release();
        //
        // ADD NEGATIVE TESTING FOR operator*()
        // --------------------------------------------------------------------

        using namespace CREATORS_TEST_NAMESPACE;

        int numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            Obj o(p);

            ASSERT(0 == numDeletes);
            o.clear();
            LOOP_ASSERT(numDeletes, 1 == numDeletes);

            ASSERT(!o && !o.ptr());
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            TObj *p;
            {
                p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
                Obj o(p);

                ASSERT(p == o.release().first);
                ASSERT(0 == numDeletes);

                ASSERT(!o && !o.ptr());
            }

            ASSERT(0 == numDeletes);
#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
            da.deleteObject(p);
#else
            delete p;
#endif
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        // testing 'release().second'
        numDeletes = 0;
        {
            TObj *p;
            {
                p =  new BSLMA_IMPLICIT_ALLOCATOR  MyTestObject(&numDeletes);
                Obj o(p);

                bslma::ManagedPtrDeleter d(o.deleter());
                bslma::ManagedPtrDeleter d2(o.release().second);
                ASSERT(0 == numDeletes);

                ASSERT(d.object()  == d2.object());
                ASSERT(d.factory() == d2.factory());
                ASSERT(d.deleter() == d2.deleter());
            }

            ASSERT(0 == numDeletes);
#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
            da.deleteObject(p);
#else
            delete p;
#endif
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

#if 0
        // testing 'deleter' accessor and 'release().second'
        numDeletes = 0;
        {
            TObj *p;
            {
                p =  new (da) MyTestObject(&numDeletes);
                Obj o(p);

                bslma::ManagedPtrDeleter d(o.deleter());
                bslma::ManagedPtrDeleter d2(o.release().second);
                ASSERT(0 == numDeletes);

                ASSERT(d.object()  == d2.object());
                ASSERT(d.factory() == d2.factory());
                ASSERT(d.deleter() == d2.deleter());
            }

            ASSERT(0 == numDeletes);
            da.deleteObject(p);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        {
            bsls::Types::Int64 numDeallocations = da.numDeallocations();
            numDeletes = 0;
            {
                SS *p = new (da) SS(&numDeletes);
                strcpy(p->d_buf, "Woof meow");

                SSObj s(p);

                // testing * and -> references
                ASSERT(!strcmp(&(*s).d_buf[5], "meow"));
                ASSERT(!strcmp(&s->d_buf[5],   "meow"));
            }
            ASSERT(da.numDeallocations() == numDeallocations + 1);
        }
#endif

      } break;
      case 14: {
        // --------------------------------------------------------------------
        // TEST ASSIGNMENT OPERATORS
        //
        // Concerns:
        //   Test swap function and all assignments operators.
        //
        //   (AJM concerns, not yet confirmed to be tested)
        //
        //   assign clears the pointer being assigned from
        //   self-assignment safe
        //   assign destroys held pointer, does not merely swap
        //   assign-with-null
        //   assign with aliased pointer
        //   assign from pointer with factory/deleter
        //   assign to pointer with factory/deleter/aliased-pointer
        //   assign from a compatible managed pointer type
        //      (e.g., ptr-to-derived, to ptr-to-base, ptr to ptr-to-const)
        //   any managed pointer can be assigned to 'bslma::ManagedPtr<void>'
        //   assign to/from an empty managed pointer, each of the cases above
        //   assigning incompatible pointers should fail to compile (hand test)
        //
        //   REFORMULATION
        //   want to be sure assignment works correctly for all combinations of
        //   assigning from and to a managed pointer with each of the following
        //   states.  Similarly, want to swap with each possible combination of
        //   each of the following states:
        //     empty
        //     simple
        //     simple with factory
        //     simple with factory and deleter
        //     simple with null factory and deleter
        //     aliased
        //     aliased (original created with factory)
        //     aliased (original created with factory and deleter)
        //
        //  In addition, assignment supports the following that 'swap' does not
        //  assignment from temporary/rvalue must be supported
        //  assignment from 'compatible' managed pointer must be supported
        //    i.e., where raw pointers would be convertible under assignment
        //
        //: X No 'bslma::ManagedPtr' method should allocate any memory.
        // Plan:
        //   TBD...
        //
        //   Test the functions in the order in which they are declared in
        //   the ManagedPtr class.
        //
        // Tested:
        //   [Just because a function is tested, we do not (yet) confirm that
        //    the testing is adequate.]
        //   void swap(ManagedPtr<ELEMENT_TYPE>& rhs);
        //   ManagedPtr& operator=(ManagedPtr &rhs);
        //   ManagedPtr& operator=(ManagedPtr_Ref<ELEMENT_TYPE> ref);
        // --------------------------------------------------------------------

        using namespace CREATORS_TEST_NAMESPACE;

        if (verbose) printf("\tTest operator=(bslma::ManagedPtr &rhs)\n");

        int numDeletes = 0;
        {
            Obj o;
            Obj o2;
            ASSERT(!o);
            ASSERT(!o2);

            o = o2;

            ASSERT(!o);
            ASSERT(!o2);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            Obj o;
            ASSERT(!o);

            o = 0;

            ASSERT(!o);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);

            Obj o(p);
            Obj o2;

            o = o2;

            ASSERT(!o);
            ASSERT(!o2);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
        }
        ASSERT(1 == numDeletes);

        numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);

            Obj o(p);

            o = 0;

            ASSERT(!o);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
        }
        ASSERT(1 == numDeletes);

        numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);

            Obj o;
            Obj o2(p);

            o = o2;

            ASSERT(!o2);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            ASSERT(o.ptr() == p);
        }
        ASSERT(1 == numDeletes);

        numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            TObj *p2 = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);

            Obj o(p);
            Obj o2(p2);

            o = o2;

            ASSERT(!o2);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);

            ASSERT(o.ptr() == p2);
        }
        ASSERT(2 == numDeletes);

        numDeletes = 0;
        {
            TObj *p =   new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            TDObj *p2 = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);

            Obj o(p);
            DObj o2(p2);

            o = o2;

            ASSERT(!o2);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);

            ASSERT(o.ptr() == p2);
        }
        ASSERT(101 == numDeletes);

        numDeletes = 0;
        {
            // this test tests creation of a ref from the same type of
            // managedPtr, then assignment to a managedptr.

            Obj o2;
            {
                TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
                Obj o(p);

                bslma::ManagedPtr_Ref<TObj> r = o;
                o2 = r;

                ASSERT(o2.ptr() == p);
            }
            ASSERT(0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            Obj o(p);
            Obj o2;

            bslma::ManagedPtr_Ref<TObj> r = o;
            o2 = r;
            ASSERT(o2);
            ASSERT(!o);
            ASSERT(0 == numDeletes);

            bslma::ManagedPtr_Ref<TObj> r2 = o;
            o2 = r2;
            ASSERT(!o2);
            ASSERT(!o);

            LOOP_ASSERT(numDeletes, 1 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            DObj o(p);
            Obj o2;

            bslma::ManagedPtr_Ref<TObj> r = o;
            o2 = r;
            ASSERT(o2);
            ASSERT(!o);
            ASSERT(0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_ASSIGN_FROM_INCOMPATIBLE_TYPE
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_ASSIGN_FROM_INCOMPATIBLE_TYPE)
            {
                bslma::ManagedPtr<int> x;
                bslma::ManagedPtr<double> y;
                y = x;  // This should fail to compile.
            }
#endif
      } break;
       case 13: {
        // --------------------------------------------------------------------
        // TESTING SWAP
        //
        // Concerns:
        //   Test swap functions.
        //
        //   (AJM concerns, not yet confirmed to be tested)
        //
        //   assign clears the pointer being assigned from
        //   self-assignment safe
        //   assign destroys held pointer, does not merely swap
        //   assign-with-null
        //   assign with aliased pointer
        //   assign from pointer with factory/deleter
        //   assign to pointer with factory/deleter/aliased-pointer
        //   assign from a compatible managed pointer type
        //      (e.g., ptr-to-derived, to ptr-to-base, ptr to ptr-to-const)
        //   any managed pointer can be assigned to 'bslma::ManagedPtr<void>'
        //   assign to/from an empty managed pointer, each of the cases above
        //   assigning incompatible pointers should fail to compile (hand test)
        //
        //   swap with self changes nothing
        //   swap two simple pointer exchanged pointer values
        //   swap two aliased pointer exchanges aliases as well as pointers
        //   swap a simple managed pointer with an empty managed pointer
        //   swap a simple managed pointer with an aliased managed pointer
        //   swap an aliased managed pointer with an empty managed pointer
        //
        //   REFORMULATION
        //   want to be sure assignment works correctly for all combinations of
        //   assigning from and to a managed pointer with each of the following
        //   states.  Similarly, want to swap with each possible combination of
        //   each of the following states:
        //     empty
        //     simple
        //     simple with factory
        //     simple with factory and deleter
        //     simple with null factory and deleter
        //     aliased
        //     aliased (original created with factory)
        //     aliased (original created with factory and deleter)
        //
        //: X No 'bslma::ManagedPtr' method should allocate any memory.
        // Plan:
        //   TBD...
        //
        //   Test the functions in the order in which they are declared in
        //   the ManagedPtr class.
        //
        // Tested:
        //   [Just because a function is tested, we do not (yet) confirm that
        //    the testing is adequate.]
        //   void swap(ManagedPtr<ELEMENT_TYPE>& rhs);
        //   ManagedPtr& operator=(ManagedPtr &rhs);
        //   ManagedPtr& operator=(ManagedPtr_Ref<ELEMENT_TYPE> ref);
        // --------------------------------------------------------------------

        using namespace CREATORS_TEST_NAMESPACE;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf(
                       "\tTest bslma::ManagedPtr::swap(bslma::ManagedPtr&)\n");

        int numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            TObj *p2 = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);

            Obj o(p);
            Obj o2(p2);

            o.swap(o2);

            ASSERT(o.ptr() == p2);
            ASSERT(o2.ptr() == p);
        }
        LOOP_ASSERT(numDeletes, 2 == numDeletes);

        if (verbose) printf("\t\tswap with empty managed pointer\n");

        numDeletes = 0;
        {
            TObj *p =  new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            Obj o(p);
            Obj o2;

            o.swap(o2);

            ASSERT(!o.ptr());
            ASSERT(o2.ptr() == p);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o.swap(o2);

            ASSERT(o.ptr() == p);
            ASSERT(!o2.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\t\tswap deleters\n");

        numDeletes = 0;
        {
            bslma::TestAllocator ta1("object1", veryVeryVeryVerbose);
            bslma::TestAllocator ta2("object2", veryVeryVeryVerbose);

            TObj *p =  new (ta1) MyTestObject(&numDeletes);
            TObj *p2 = new (ta2) MyTestObject(&numDeletes);

            Obj o(p, &ta1);
            Obj o2(p2, &ta2);

            o.swap(o2);

            ASSERT(o.ptr() == p2);
            ASSERT(o2.ptr() == p);

            ASSERT(&ta2 == o.deleter().factory());
            ASSERT(&ta1 == o2.deleter().factory());
        }
        LOOP_ASSERT(numDeletes, 2 == numDeletes);

        if (verbose) printf("\t\tswap aliases\n");

        numDeletes = 0;
        {
            bslma::TestAllocator ta1("object1", veryVeryVeryVerbose);
            bslma::TestAllocator ta2("object2", veryVeryVeryVerbose);

            int * p3 = new (ta2) int;
            *p3 = 42;

            TObj *p =  new (ta1) MyTestObject(&numDeletes);
            MyDerivedObject d2(&numDeletes);

            bslma::ManagedPtr<int> o3(p3, &ta2);
            Obj o(p, &ta1);
            Obj o2(o3, &d2);

            o.swap(o2);

            ASSERT( o.ptr() == &d2);
            ASSERT(o2.ptr() ==   p);

            ASSERT(p3 ==  o.deleter().object());
            ASSERT( p == o2.deleter().object());
            ASSERT(&ta2 ==  o.deleter().factory());
            ASSERT(&ta1 == o2.deleter().factory());
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_SWAP_FOR_DIFFERENT_TYPES
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_SWAP_FOR_DIFFERENT_TYPES)
            {
                // confirm that the various implicit conversions in this
                // component do not accidentally introduce a dangerous 'swap'.
                bslma::ManagedPtr<int> x;
                bslma::ManagedPtr<double> y;
                x.swap(y);  // should not compile
                y.swap(x);  // should not compile

                bslma::ManagedPtr<MyTestObject> b;
                bslma::ManagedPtr<MyDerivedObject> d;
                b.swap(d);  // should not compile
                d.swap(b);  // should not compile

                using std::swap;
                swap(x, y);  // should not compile
                swap(b, d);  // should not compile
            }
#endif
      } break;
      case 12: {
        // --------------------------------------------------------------------
        // ALIAS SUPPORT TEST
        //
        // Concerns:
        //   managed pointer can hold an alias
        //
        //   'ptr' returns the alias pointer, and not the managed pointer
        //
        //   correct deleter is run when an aliased pointer is destroyed
        //
        //   appropriate object is cleared/deleters run when assigning to/from
        //   an aliased managed pointer
        //
        //   a managed pointer can alias itself
        //
        //   alias type need not be the same as the managed type (often isn't)
        //
        //   aliasing a null pointer clears the managed pointer, releasing any
        //   previously held object
        //
        //: X No 'bslma::ManagedPtr' method should allocate any memory.
        //
        // Plan:
        //   TBD...
        //
        // Tested:
        //   bslma::ManagedPtr(bslma::ManagedPtr<OTHER> &alias, TYPE *ptr)
        //   void loadAlias(bslma::ManagedPtr<OTHER> &alias, TYPE *ptr)

        // TEST SCENARIOS for 'loadAlias'
        //   Alias an existing state:
        //     Run through the function table for test case 'load'
        //     Test 1:
        //       Load a known state into an empty managed pointer
        //       call 'loadAlias' on a second empty managed pointer
        //       Check aliased state, and original managed pointer
        //         negative test alias with a null pointer value
        //         negative test if aliased managed pointer is empty
        //       Check no memory allocated by aliasing
        //       Run destructor and validate
        //     Test 2:
        //       Load a known state into an empty managed pointer
        //       call 'loadAlias' on a second empty managed pointer
        //       Check aliased state, and original managed pointer
        //       call 'loadAlias' again on a third empty managed pointer
        //       Check new aliased state, and first aliased managed pointer
        //       Check no memory allocated by aliasing
        //       Run destructor and validate
        //     Test 3: (to be written)
        //       Create an alias
        //       Check aliased state, and original managed pointer
        //       run another 'load' function and check alias destroys correctly
        //       destroy 'load'ed managed pointer, validating results
        // --------------------------------------------------------------------

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\nTesting 'loadAlias' overloads"
                            "\n-----------------------------\n");

        {
            if (veryVerbose)
                printf("Testing bslma::ManagedPtr<MyTestObject>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                 "Testing bslma::ManagedPtr<const MyTestObject>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_CONST_BASE_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_CONST_BASE_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_CONST_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {   // TBD Create a further derived class to allow this aliasing test
            //     case to compile.
            //if (veryVerbose) printf(
            //      "Testing bslma::ManagedPtr<MyDerivedObject>::loadAlias\n");

            //testLoadAliasOps1<MyDerivedObject>(L_, TEST_DERIVED_ARRAY);
            //testLoadAliasOps2<MyDerivedObject>(L_, TEST_DERIVED_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                        printf("Testing bslma::ManagedPtr<void>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_VOID_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_VOID_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                  printf("Testing bslma::ManagedPtr<const void>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_CONST_VOID_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_CONST_VOID_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_CONST_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE)
        {
            if (veryVerbose)
                        printf("Testing bslma::ManagedPtr<Base>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE0_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE0_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE0_ARRAY);
        }
#endif
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                       printf("Testing bslma::ManagedPtr<Base2>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE2_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE2_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE2_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        using namespace CREATORS_TEST_NAMESPACE;

#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
        int numDeletes = 0;
        {
            SS *p = new (da) SS(&numDeletes);
            strcpy(p->d_buf, "Woof meow");

            SSObj s(p);
            ChObj c(s, &p->d_buf[5]);

            ASSERT(!s); // should not be testing operator! until test 13

            ASSERT(!strcmp(c.ptr(), "meow"));

            ASSERT(0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);


        bsls::Types::Int64 numDeallocations = da.numDeallocations();
        numDeletes = 0;
        {
            SS *p = new (da) SS(&numDeletes);
            strcpy(p->d_buf, "Woof meow");
            char *pc = (char *) da.allocate(5);
            strcpy(pc, "Werf");

            SSObj s(p);
            ChObj c(pc);

            ASSERT(da.numDeallocations() == numDeallocations);
            c.loadAlias(s, &p->d_buf[5]);
            ASSERT(da.numDeallocations() == numDeallocations + 1);

            ASSERT(!s); // should not be testing operator! until test 13

            ASSERT(!strcmp(c.ptr(), "meow"));
        }
        ASSERT(da.numDeallocations() == numDeallocations + 2);
#else
        int numDeletes = 0;
        {
            SS *p = new SS(&numDeletes);
            strcpy(p->d_buf, "Woof meow");

            SSObj s(p);
            ChObj c(s, &p->d_buf[5]);

            ASSERT(!s); // should not be testing operator! until test 13

            ASSERT(!strcmp(c.ptr(), "meow"));

            ASSERT(0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);


        bsls::Types::Int64 numDeallocations = da.numDeallocations();
        numDeletes = 0;
        {
            SS *p = new SS(&numDeletes);
            strcpy(p->d_buf, "Woof meow");
            char *pc = (char *) da.allocate(5);
            strcpy(pc, "Werf");

            SSObj s(p);
            ChObj c(pc, &da);

            ASSERT(da.numDeallocations() == numDeallocations);
            c.loadAlias(s, &p->d_buf[5]);
            ASSERT(da.numDeallocations() == numDeallocations + 1);

            ASSERT(!s); // should not be testing operator! until test 13

            ASSERT(!strcmp(c.ptr(), "meow"));
        }
        ASSERT(da.numDeallocations() == numDeallocations + 1);
#endif
      } break;
      case 11: {
        // --------------------------------------------------------------------
        // MOVE-CONSTRUCTION
        //
        // Concerns:
        //: 1 No constructor nor conversion operator allocates any memory from
        //:   the default or global allocators.
        //:
        //: 2 Each constructor takes ownership of the passed managed object.
        //:
        //: 3 Move construction for lvalues directly invokes the copy/move-
        //:   constructor (or constructor template) to take ownership away from
        //:   the source managed pointer object.
        //:
        //: 4 Move semantics for temporary objects (rvalues) are supported
        //:   through an implicit conversion to 'bslma::ManagedPtr_Ref', and
        //:   the single argument (implicit) constructor taking ownership from
        //:   such a managed reference.
        //:
        //: 5 const-qualified objects cannot be moved from (compile-fail test).
        //:
        //: 6 Both lvalue and rvalue objects of 'bslma::ManagedPtr' types can
        //:   implicitly convert to a 'bslma::ManagedPtr_Ref' of any compatible
        //:   type, i.e., where a pointer to the specified '_Ref' type may be
        //:   converted from a pointer to the specified 'Managed' type.
        //:
        //: 7 A 'bslma::ManagedPtr' object is left in an empty state after
        //:   being supplied as the source to a move operation.
        //:
        //: 8 A 'bslma::ManagedPtr' object holding an object of a most-derived
        //:   class can correctly move to a 'bslma::ManagedPtr' holding one of
        //:   its non-leftmost base classes.
        //
        // Plan:
        //   First we test the conversion operator, including compile-fail test
        //   for incompatible types
        //
        //   Next we test construction from a 'bslma::ManagedPtr_Ref' object
        //
        //   Finally we test the tricky combinations of invoking the (lvalue)
        //   move constructor, including with rvalues, and values of different
        //   target types.
        //
        // Tested:
        //   operator bslma::ManagedPtr_Ref<OTHER_TYPE>();
        //   bslma::ManagedPtr(bslma::ManagedPtr_Ref<ELEMENT_TYPE> ref);
        //   bslma::ManagedPtr(bslma::ManagedPtr &original);
        // --------------------------------------------------------------------

        using namespace CREATORS_TEST_NAMESPACE;

        bslma::TestAllocator ta("object", veryVeryVeryVerbose);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose)
                    printf("\tTest operator bslma::ManagedPtr_Ref<OTHER>()\n");

        LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
        {
            bslma::TestAllocatorMonitor gam(&globalAllocator);
            bslma::TestAllocatorMonitor dam(&da);

            int numDeletes = 0;

            {
                TObj x(&numDeletes);
                Obj  o(&x, 0, countedNilDelete);

                bslma::ManagedPtr_Ref<TObj> r = o;
                // Check no memory is allocated/released and no deleters run
                LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
                ASSERT(0 == numDeletes);

                // check the pointer reference an object with the correct data
                ASSERT(&x == r.base()->pointer());
                ASSERT(&x == r.base()->deleter().object());
                ASSERT(0 == r.base()->deleter().factory());
                ASSERT(&countedNilDelete == r.base()->deleter().deleter());

                // finally, check the address of the pointed-to object lies
                // within 'o', as we cannot directly query the address of the
                // private member
                const void *p1 = &o;
                const void *p2 = reinterpret_cast<const unsigned char *>(p1) +
                                                                     sizeof(o);
                const void *pRef = r.base();
                LOOP3_ASSERT(p1, pRef, p2, p1 <= pRef && pRef < p2);
            }
            LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
            ASSERT(dam.isInUseSame());
            ASSERT(dam.isMaxSame());
            ASSERT(gam.isInUseSame());
            ASSERT(gam.isMaxSame());

            g_deleteCount = 0;
            numDeletes = 0;
            {
                // To test conversion from an rvalue, we must bind the
                // the temporary to a function argument in order to prolong the
                // lifetime of the temporary until after testing is complete.
                // We must bind the temporary to a 'bslma::ManagedPtr_Ref' and
                // not a whole 'bslma::ManagedPtr' because we are testing an
                // implementation detail of that move-constructor that would be
                // invoked.
                struct Local {
                    static void test(void * px,
                                     bslma::ManagedPtr_Ref<TObj> r)
                    {
                        LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);

                        ASSERT(px == r.base()->pointer());
                        ASSERT(px == r.base()->deleter().object());
                        ASSERT(0 == r.base()->deleter().factory());
                        ASSERT(&countedNilDelete ==
                                                r.base()->deleter().deleter());
                    }
                };

                TObj x(&numDeletes);
                Local::test( &x, (Obj(&x, 0, countedNilDelete)));
            }
            LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
            ASSERT(dam.isInUseSame());
            ASSERT(dam.isMaxSame());
            ASSERT(gam.isInUseSame());
            ASSERT(gam.isMaxSame());

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_CONVERT_TO_REF_FROM_CONST
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_CONVERT_TO_REF_FROM_CONST)
            {
                TObj x(&numDeletes);
                const Obj o(&x, 0, countedNilDelete);

                bslma::ManagedPtr_Ref<TObj> r = o;   // should not compile
                LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
                ASSERT(0 == numDeletes);
            }
#endif
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tbslma::ManagedPtr(bslma::ManagedPtr &donor)\n");

        {
            bslma::TestAllocatorMonitor gam(&globalAllocator);
            bslma::TestAllocatorMonitor dam(&da);

            g_deleteCount = 0;
            int numDeletes = 0;
            {
                TObj x(&numDeletes);
                Obj  o(&x, 0, countedNilDelete);
                ASSERT(&x == o.ptr());

                Obj o2(o);
                ASSERT( 0 ==  o.ptr());
                ASSERT(&x == o2.ptr());
                ASSERT(&x == o2.deleter().object());
                ASSERT( 0 == o2.deleter().factory());
                ASSERT(&countedNilDelete == o2.deleter().deleter());
            }

            LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
            ASSERT(dam.isInUseSame());
            ASSERT(dam.isMaxSame());
            ASSERT(gam.isInUseSame());
            ASSERT(gam.isMaxSame());

            g_deleteCount = 0;
            numDeletes = 0;
            {
                TObj x(&numDeletes);
                Obj  o = Obj(&x, 0, countedNilDelete);
                ASSERT(&x == o.ptr());

                Obj o2(o);
                ASSERT( 0 ==  o.ptr());
                ASSERT(&x == o2.ptr());
                ASSERT(&x == o2.deleter().object());
                ASSERT( 0 == o2.deleter().factory());
                ASSERT(&countedNilDelete == o2.deleter().deleter());
            }

            LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);
            LOOP_ASSERT(numDeletes, 1 == numDeletes);
            ASSERT(dam.isInUseSame());
            ASSERT(dam.isMaxSame());
            ASSERT(gam.isInUseSame());
            ASSERT(gam.isMaxSame());

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_MOVE_CONSTRUCT_FROM_CONST
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_MOVE_CONSTRUCT_FROM_CONST)
            {
                TObj x(&numDeletes);
                const Obj  o(&x, 0, countedNilDelete);
                ASSERT(&x == o.ptr());

                Obj o2(o);  // should not compile
                ASSERT(!"The preceding line should not have compiled");
            }
#endif
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf(
        "\tTest bslma::ManagedPtr(bslma::ManagedPtr_Ref<ELEMENT_TYPE> ref)\n");

        int numDeletes = 0;
        {
            // this cast tests both a cast while creating the ref,
            // and the constructor from a ref.

            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            DObj o(p);

            ASSERT(o);
            ASSERT(o.ptr() == p);

            bslma::ManagedPtr_Ref<TObj> r = o;
            ASSERT(o);
            Obj o2(r);

            ASSERT(!o && !o.ptr());
            ASSERT(0 == numDeletes);

            ASSERT(o2.ptr() == p);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        numDeletes = 0;
        {
            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            DObj d(p);
            ASSERT(d.ptr() == p);

            Obj o(d);
            ASSERT(o.ptr() == p);
            ASSERT(0 == d.ptr());
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTesting moving to non-leftmost base\n");
        {
            CompositeInt3 derived;
            bslma::ManagedPtr<CompositeInt3> pD(
                                          &derived,
                                          0,
                                          &bslma::ManagedPtrUtil::noOpDeleter);
            int testVal  = pD->data();
            int testVal2 = pD->data2();
            LOOP_ASSERT(testVal,  3 == testVal);  // pD->data()
            LOOP_ASSERT(testVal2, 6 == testVal2); // pD->data2()
            LOOP_ASSERT(pD->d_data, 3 == pD->d_data);

            bslma::ManagedPtr<BaseInt2> pB(pD);  // cannot use '=' form
            ASSERT(0 == pD.ptr());

            testVal  = pB->data();
            testVal2 = pB->data2();
            LOOP_ASSERT(testVal,  3 == testVal);  // pB->data()
            LOOP_ASSERT(testVal2, 6 == testVal2); // pB->data2()
            LOOP_ASSERT(pB->d_data, 2 == pB->d_data);

            // After testing construction, test assignment
            bslma::ManagedPtr<CompositeInt3> pD2(
                                          &derived,
                                          0,
                                          &bslma::ManagedPtrUtil::noOpDeleter);
            // sanity checks only
            testVal  = pD2->data();
            testVal2 = pD2->data2();
            LOOP_ASSERT(testVal,  3 == testVal);  // pD2->data()
            LOOP_ASSERT(testVal2, 6 == testVal2); // pD2->data2()
            LOOP_ASSERT(pD2->d_data, 3 == pD2->d_data);

            pB = pD2;
            ASSERT(0 == pD2.ptr());

            testVal  = pB->data();
            testVal2 = pB->data2();
            LOOP_ASSERT(testVal,  3 == testVal);  // pB->data()
            LOOP_ASSERT(testVal2, 6 == testVal2); // pB->data2()
            LOOP_ASSERT(pB->d_data, 2 == pB->d_data);
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// examples to demonstrate:
        // Moving from lvalues:
        //   derived->base
        //   no-cv -> const
        //   anything -> void
        //
        // Moving from rvalues:
        //   as above, plus...
        //   rvalue of same type

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_INCOMPATIBLE_POINTERS
#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_INCOMPATIBLE_POINTERS
        {
            int x;
            bslma::ManagedPtr<int> i_ptr(&x, 0, &countedNilDelete);
            bslma::ManagedPtr<double> d_ptr(i_ptr);

            struct Local_factory {
                static bslma::ManagedPtr<double> exec()
                {
                    return bslma::ManagedPtr<double>();
                }
            };

            bslma::ManagedPtr<long> l_ptr(Local_factory::exec());
        }

        // Additional failures to demonstrate
        //   base -> derived type (a likely user error)
        //   const -> non-const
        //   void -> anything but void
#endif
      } break;
      case 10: {
        // --------------------------------------------------------------------
        // CREATORS WITH FACTORY OR DELETER
        //
        // Concerns:
        //: 1 No constructor allocates any memory from the default or global
        //:   allocators.
        //:
        //: 2 Each constructor takes ownership of the passed managed object.
        //:
        //: 3 Each constructor establishes the supplied 'deleter', unless the
        //:   specified managed object has a null pointer value.
        //:
        //: 4 Each constructor ASSERTs in appropriate build modes when passed
        //:   a null pointer for the deleter, but a non-null pointer to the
        //:   managed object.
        //:
        //: 5 It must be possible to pass a null-pointer constant for the
        //:   'factory' argument when the specified 'deleter' will use only the
        //:   managed pointer value.
        //
        //   Exercise each declared constructors of ManagedPtr (other than
        //   those already tested in an earlier test case; those constructors
        //   that implement move semantics; and the constructor that enables
        //   aliasing).  Note that the primary accessor, 'ptr', cannot be
        //   considered to be validated until after testing the alias support,
        //   see test case 11.
        //
        // Plan:
        //   TBD...
        //
        //   Go through the constructors in the order in which they are
        //   declared in the ManagedPtr class and exercise all of them.
        //
        //   Remember to pass '0' as a null-pointer literal to all arguments
        //   that accept pointers (with negative testing if that is out of
        //   contract).
        //
        // Tested:
        //   bslma::ManagedPtr(TARGET_TYPE *ptr)
        //   bslma::ManagedPtr(TARGET_TYPE *ptr, FACTORY *factory)
        //   bslma::ManagedPtr(TARGET_TYPE *, void *, DeleterFunc)
        //   bslma::ManagedPtr(TARGET_TYPE *,
        //                      nullptr_t,
        //                           void(*)(TARGET_BASE *, void *))
        //   bslma::ManagedPtr(TARGET_TYPE *,
        //                        FACTORY *,
        //                           void(*)(TARGET_BASE *, FACTORY_BASE *))
        // --------------------------------------------------------------------

        if (verbose) printf("\nTesting 'load' overloads"
                            "\n------------------------\n");

        {
            if (veryVerbose)
                     printf("Testing bslma::ManagedPtr<MyTestObject>::load\n");

            testConstructors(L_, TEST_POLICY_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
               printf("Testing bslma::ManagedPtr<const MyTestObject>::load\n");

            testConstructors(L_, TEST_POLICY_CONST_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                  printf("Testing bslma::ManagedPtr<MyDerivedObject>::load\n");

            testConstructors(L_, TEST_POLICY_DERIVED_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf("Testing bslma::ManagedPtr<void>::load\n");

            testConstructors(L_, TEST_POLICY_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                       printf("Testing bslma::ManagedPtr<const void>::load\n");

            testConstructors(L_, TEST_POLICY_CONST_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE)
        {
            if (veryVerbose) printf("Testing bslma::ManagedPtr<Base>::load\n");

            testConstructors(L_, TEST_POLICY_BASE0_ARRAY);
        }
#endif
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                            printf("Testing bslma::ManagedPtr<Base2>::load\n");

            testConstructors(L_, TEST_POLICY_BASE2_ARRAY);
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        using namespace CREATORS_TEST_NAMESPACE;

        bslma::TestAllocator ta("object", veryVeryVeryVerbose);

        if (verbose) printf("\tTest valid pointer passed to void*\n");

        int numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tconst-qualified int\n");

            bslma::TestAllocatorMonitor dam(&da);
#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
            {
                const int *p = new (da) const int(0);

                bslma::TestAllocatorMonitor dam2(&da);
                bslma::ManagedPtr<const int> o(p);

                ASSERT(o.ptr() == p);
                ASSERT(dam2.isInUseSame());
            }
            ASSERT(dam.isTotalUp());
            ASSERT(dam.isInUseSame());
#else
            {
                bslma::TestAllocatorMonitor dam2(&da);

                const int *p = new const int(0);
                bslma::ManagedPtr<const int> o(p);

                ASSERT(o.ptr() == p);
                ASSERT(dam2.isInUseSame());
            }
            ASSERT(!dam.isTotalUp());
            ASSERT(dam.isInUseSame());
#endif
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tint -> const int conversion\n");

            bslma::TestAllocatorMonitor dam(&da);
#if defined(BSLMA_USE_OLD_DEFAULT_ALLOCATOR_SEMANTICS_BEFORE_DRQS27411521)
            {
                int *p = new (da) int;

                bslma::TestAllocatorMonitor dam2(&da);
                bslma::ManagedPtr<const int> o(p);

                ASSERT(o.ptr() == p);
                ASSERT(dam2.isInUseSame());
            }
            ASSERT(dam.isTotalUp());
            ASSERT(dam.isInUseSame());
#else
            {
                bslma::TestAllocatorMonitor dam2(&da);

                int *p = new int;
                bslma::ManagedPtr<const int> o(p);

                ASSERT(o.ptr() == p);
                ASSERT(dam2.isInUseSame());
            }
            ASSERT(!dam.isTotalUp());
            ASSERT(dam.isInUseSame());
#endif
        }
        ASSERT(0 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTest bslma::ManagedPtr(ELEMENT_TYPE *ptr,"
                             " bsl::nullptr_t,"
                             " void(*)(ELEMENT_TYPE *, void*));\n");

        numDeletes = 0;
        LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
        {
            bslma::TestAllocatorMonitor tam(&ta);

            MyTestObject obj(&numDeletes);
            Obj o(&obj, 0, &templateNilDelete<MyTestObject>);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);
        LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_CONSTRUCT_FROM_INCOMPATIBLE_POINTER
#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_CONSTRUCT_FROM_INCOMPATIBLE_POINTER
        // This segment of the test case examines the quality of compiler
        // diagnostics when trying to create a 'bslma::ManagedPtr' object with
        // a pointer that it not convertible to a pointer of the type that the
        // smart pointer is managing.

        if (verbose) printf("\tTesting compiler diagnostics*\n");

        // distint, unrelated types
        numDeletes = 0;
        {
            double *p = new (da) double;
            Obj o(p);

//            ASSERT(o.ptr() == p);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        // const-conversion
        numDeletes = 0;
        {
            const MyTestObject *p = new (da) MyTestObject(&numDeletes);
            Obj o(p);

//            ASSERT(o.ptr() == p);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            const MyTestObject *p = new (da) MyTestObject(&numDeletes);
            VObj o(p);

            ASSERT(o.ptr() == p);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);
#endif

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_FACTORY
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_FACTORY)
        {
            int i = 0;
            bslma::ManagedPtr<int> x(&i, 0);
            bslma::ManagedPtr<int> y( 0, 0);

            bslma::Allocator * pNullAlloc = 0;
            bslma::ManagedPtr<int> z(0, pNullAlloc);
        }
#endif

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_FACTORY
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_FACTORY)
        {
            int *i = 0;
            bslma::ManagedPtr<const int> x(&i, 0);
            bslma::ManagedPtr<int> y( 0, 0);  // allow this?

            bslma::Allocator * pNullAlloc = 0;
            bslma::ManagedPtr<const int> z(0, pNullAlloc);  // allow this?
        }
#endif

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_DELETER
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_NULL_DELETER)
        {
            int *i = 0;
            bslma::ManagedPtr<const int> x(i, 0, 0);
            bslma::ManagedPtr<const int> y(0, 0, 0);  // allow this?

            // These are currently runtime (UB) failures, rather than
            // compile-time errors.
            bslma::Allocator * pNullAlloc = 0;
            bslma::ManagedPtr<const int> z( i, pNullAlloc, 0);
            bslma::ManagedPtr<const int> zz(0, pNullAlloc, 0);  // allow this?
        }
#endif
      } break;
      case 9: {
        // --------------------------------------------------------------------
        // TESTING ACCESSORS
        //
        // Concerns:
        //   That all accessors work properly.  The 'ptr' accessor has
        //   already been substantially tested in previous tests.
        //   The unspecified bool conversion evaluates as expected in all
        //     circumstances: if/while/for, (implied) operator!
        //   All accessors work on 'const'- qualified objects
        //   All accessors can be called for 'bslma::ManagedPtr<void>'
        //   All accessors return expected values when a 'bslma::ManagedPtr'
        //     has been aliased
        //   'operator*' should assert in SAFE builds for empty pointers
        //   'deleter' should assert in SAFE builds for empty pointers
        //   'operator*' should be well-formed, but not callable for
        //     'bslma::ManagedPtr<void>'.
        //
        //: X No 'bslma::ManagedPtr' method should allocate any memory.
        //
        // Plan:
        //   Test each accessor for the expected value on each of the following
        //   cases:
        //     empty
        //     simple
        //     simple with factory
        //     simple with factory and deleter
        //     simple with null factory and deleter
        //     aliased
        //     aliased (original created with factory)
        //     aliased (original created with factory and deleter)
        //
        //  For 'bslma::ManagedPtr<void>', test syntax of 'operator*' in an
        //    unevaluated context, such as a 'typeid' expression.
        //
        //  Test that illegal expressions cannot compile in compile-fail tests,
        //  guarded by #ifdefs, where necessary.
        //
        // Tested:
        //   operator BoolType() const;
        //   TYPE& operator*() const;
        //   TYPE *operator->() const;
        //   TYPE *ptr() const;
        //   const bslma::ManagedPtrDeleter& deleter() const;
        //   (implicit operator!() via operator BoolType())
        // --------------------------------------------------------------------

        bslma::TestAllocator ta("object", veryVeryVeryVerbose);

        if (verbose) printf("\tTest accessors on empty object\n");

        int numDeletes = 0;
        {
            const Obj o;
            const bslma::ManagedPtrDeleter del;

            validateManagedState(L_, o, 0, del);
        }

        LOOP_ASSERT(numDeletes, 0 == numDeletes);
        {
            const VObj o;
            const bslma::ManagedPtrDeleter del;

            validateManagedState(L_, o, 0, del);
            // The following 'typeid' fails on Unix compilers, but should be
            // an unevaluated operand, and so safely invokable.
            //typeid(*o); // should parse, even if it cannot be called
        }

        LOOP_ASSERT(numDeletes, 0 == numDeletes);
        {
            const bslma::ManagedPtr<const void> o(0);
            const bslma::ManagedPtrDeleter del;

            validateManagedState(L_, o, 0, del);
            // The following 'typeid' fails on Unix compilers, but should be
            // an unevaluated operand, and so safely invokable.
            //typeid(*o); // should parse, even if it cannot be called
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTest accessors on simple object\n");

        typedef bslma::ManagedPtr_FactoryDeleter<TObj,bslma::Allocator>
                                                        TestcaseFactoryDeleter;
        LOOP_ASSERT(numDeletes, 0 == numDeletes);
        {
            Obj o;
            TObj *p = new (da) MyTestObject(&numDeletes);
            o.load(p);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &da,
                                             &TestcaseFactoryDeleter::deleter);

            validateManagedState(L_, o, p, del);

            Obj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);

        numDeletes = 0;
        {
            VObj o;
            TObj *p = new (da) MyTestObject(&numDeletes);
            o.load(p);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &da,
                                             &TestcaseFactoryDeleter::deleter);

            validateManagedState(L_, o, p, del);

            VObj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);

        numDeletes = 0;
        {
            bslma::ManagedPtr<const void> o;
            TObj *p = new (da) MyTestObject(&numDeletes);
            o.load(p);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &da,
                                             &TestcaseFactoryDeleter::deleter);

            validateManagedState(L_, o, p, del);

            bslma::ManagedPtr<const void> oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf(
                        "\tTest accessors on simple object using a factory\n");

        bslma::TestAllocatorMonitor tam(&ta);
        numDeletes = 0;
        {
            Obj o;
            TObj *p = new (ta) MyTestObject(&numDeletes);
            o.load(p, &ta);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &ta,
                                             &TestcaseFactoryDeleter::deleter);

            bslma::TestAllocatorMonitor tam2(&ta);

            validateManagedState(L_, o, p, del);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());

            Obj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        ASSERT(tam.isInUseSame());

        numDeletes = 0;
        {
            VObj o;
            TObj *p = new (ta) MyTestObject(&numDeletes);
            o.load(p, &ta);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &ta,
                                             &TestcaseFactoryDeleter::deleter);

            bslma::TestAllocatorMonitor tam2(&ta);

            validateManagedState(L_, o, p, del);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());

            VObj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        ASSERT(tam.isInUseSame());

        numDeletes = 0;
        {
            bslma::ManagedPtr<const void> o;
            TObj *p = new (ta) MyTestObject(&numDeletes);
            o.load(p, &ta);
            const bslma::ManagedPtrDeleter del(
                                              p,
                                             &ta,
                                             &TestcaseFactoryDeleter::deleter);

            bslma::TestAllocatorMonitor tam2(&ta);

            validateManagedState(L_, o, p, del);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());

            bslma::ManagedPtr<const void> oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            ASSERT(tam2.isInUseSame());
            ASSERT(tam2.isMaxSame());
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        ASSERT(tam.isInUseSame());

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf(
         "\tTest accessors on simple object using a deleter but no factory\n");

        g_deleteCount = 0;
        numDeletes = 0;
        {
            Obj o;
            TObj obj(&numDeletes);
            o.load(&obj, 0, &countedNilDelete);
            const bslma::ManagedPtrDeleter del(&obj, 0, &countedNilDelete);

            validateManagedState(L_, o, &obj, del);

            Obj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);

        g_deleteCount = 0;
        numDeletes = 0;
        {
            VObj o;
            TObj obj(&numDeletes);
            o.load(&obj, 0, &countedNilDelete);
            const bslma::ManagedPtrDeleter del(&obj, 0, &countedNilDelete);

            validateManagedState(L_, o, &obj, del);

            VObj oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);

        g_deleteCount = 0;
        numDeletes = 0;
        {
            bslma::ManagedPtr<const void> o;
            TObj obj(&numDeletes);
            o.load(&obj, 0, &countedNilDelete);
            const bslma::ManagedPtrDeleter del(&obj, 0, &countedNilDelete);

            validateManagedState(L_, o, &obj, del);

            bslma::ManagedPtr<const void> oD;
            {
                MyDerivedObject d(&numDeletes);
                oD.loadAlias(o, &d);
                validateManagedState(L_, oD, &d, del);
            }
            LOOP_ASSERT(numDeletes, 100 == numDeletes);
            LOOP_ASSERT(g_deleteCount, 0 == g_deleteCount);
        }
        LOOP_ASSERT(numDeletes, 101 == numDeletes);
        LOOP_ASSERT(g_deleteCount, 1 == g_deleteCount);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf(
       "\tTest accessors on simple object using both a factory and deleter\n");

        numDeletes = 0;
        {
            bslma::ManagedPtr<int> o;
            IncrementIntFactory factory;
            o.load(&numDeletes, &factory, &incrementIntDeleter);
            const bslma::ManagedPtrDeleter del(&numDeletes,
                                              &factory,
            (bslma::ManagedPtrDeleter::Deleter)&incrementIntDeleter);

            validateManagedState(L_, o, &numDeletes, del);

            bslma::ManagedPtr<int> o2;
            int i2 = 0;
            {
                o2.loadAlias(o, &i2);
                validateManagedState(L_, o2, &i2, del);
            }
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            VObj o;
            IncrementIntFactory factory;
            o.load(&numDeletes, &factory, &incrementIntDeleter);
            const bslma::ManagedPtrDeleter del(&numDeletes,
                                              &factory,
            (bslma::ManagedPtrDeleter::Deleter)&incrementIntDeleter);

            validateManagedState(L_, o, &numDeletes, del);

            bslma::ManagedPtr<int> o2;
            int i2 = 0;
            {
                o2.loadAlias(o, &i2);
                validateManagedState(L_, o2, &i2, del);
            }
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        numDeletes = 0;
        {
            bslma::ManagedPtr<const void> o;
            IncrementIntFactory factory;
            o.load(&numDeletes, &factory, &incrementIntDeleter);
            const bslma::ManagedPtrDeleter del(&numDeletes,
                                              &factory,
            (bslma::ManagedPtrDeleter::Deleter)&incrementIntDeleter);

            validateManagedState(L_, o, &numDeletes, del);

            bslma::ManagedPtr<int> o2;
            int i2 = 0;
            {
                o2.loadAlias(o, &i2);
                validateManagedState(L_, o2, &i2, del);
            }
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_DEREFERENCE_VOID_PTR
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_DEREFERENCE_VOID_PTR)
            {
                int x;
                VObj p(&x);
                *p;

                bslma::ManagedPtr<const void> p2(&x);
                *p2;
            }
#endif
      } break;
      case 8: {
        // --------------------------------------------------------------------
        // TESTING loadAlias
        //
        // Concerns:
        //   managed pointer can hold an alias
        //   'ptr' returns the alias pointer, and not the managed pointer
        //   correct deleter is run when an aliased pointer is destroyed
        //   appropriate object is cleared/deleters run when assigning to/from
        //       an aliased managed pointer
        //   a managed pointer can alias itself
        //   alias type need not be the same as the managed type (often isn't)
        //   aliasing a null pointer clears the managed pointer, releasing any
        //       previously held object
        //
        //: X No 'bslma::ManagedPtr' method should allocate any memory.
        //
        // Plan:
        //   TBD...
        //
        // Tested:
        //   void loadAlias(bslma::ManagedPtr<OTHER> &alias, TYPE *ptr)

        // TEST SCENARIOS for 'loadAlias'
        //   Alias an existing state:
        //     Run through the function table for test case 'load'
        //     Test 1:
        //       Load a known state into an empty managed pointer
        //       call 'loadAlias' on a second empty managed pointer
        //       Check aliased state, and original managed pointer
        //         negative test alias with a null pointer value
        //         negative test if aliased managed pointer is empty
        //       Check no memory allocated by aliasing
        //       Run destructor and validate
        //     Test 2:
        //       Load a known state into an empty managed pointer
        //       call 'loadAlias' on a second empty managed pointer
        //       Check aliased state, and original managed pointer
        //       call 'loadAlias' again on a third empty managed pointer
        //       Check new aliased state, and first aliased managed pointer
        //       Check no memory allocated by aliasing
        //       Run destructor and validate
        //     Test 3: (to be written)
        //       Create an alias
        //       Check aliased state, and original managed pointer
        //       run another 'load' function and check alias destroys correctly
        //       destroy 'load'ed managed pointer, validating results
        // --------------------------------------------------------------------

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\nTesting 'loadAlias' overloads"
                            "\n-----------------------------\n");

        {
            if (veryVerbose)
                printf("Testing bslma::ManagedPtr<MyTestObject>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                 "Testing bslma::ManagedPtr<const MyTestObject>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_CONST_BASE_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_CONST_BASE_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_CONST_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                    "Testing bslma::ManagedPtr<MyDerivedObject>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_DERIVED_ARRAY);
            //testLoadAliasOps2(L_, TEST_POLICY_DERIVED_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_DERIVED_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                        printf("Testing bslma::ManagedPtr<void>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_VOID_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_VOID_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                  printf("Testing bslma::ManagedPtr<const void>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_CONST_VOID_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_CONST_VOID_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_CONST_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE)
        {
            if (veryVerbose)
                        printf("Testing bslma::ManagedPtr<Base>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE0_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE0_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE0_ARRAY);
        }
#endif

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose)
                       printf("Testing bslma::ManagedPtr<Base2>::loadAlias\n");

            testLoadAliasOps1(L_, TEST_POLICY_BASE2_ARRAY);
            testLoadAliasOps2(L_, TEST_POLICY_BASE2_ARRAY);
            testLoadAliasOps3(L_, TEST_POLICY_BASE2_ARRAY);
        }

      } break;
      case 7: {
        // --------------------------------------------------------------------
        // Testing 'load' overloads
        //
        // Concerns:
        //: 1 Calling 'load' on an empty managed pointer assigns ownership of
        //:   the pointer passed as the argument.
        //:
        //: 2 Calling 'load' on a 'bslma::ManagedPtr' that owns a non-null
        //:   pointer destroys the referenced object, and takes ownership of
        //:   the new one.
        //:
        //: 3 Calling load with a null pointer, or no argument, causes a
        //:   'bslma::ManagedPtr' object to destroy any managed object, without
        //:   owning a new one.
        //:
        //: 4 'bslma::ManagedPtr<void>' can load a pointer to any other type,
        //:   owning the pointer and deducing a deleter that will correctly
        //:   destroy the pointed-to object.
        //:
        //: 5 'bslma::ManagedPtr<void>' can load a true 'void *' pointer only
        //:   if an appropriate factory or deleter function is also passed.
        //:   The single argument 'load(void *) should fail to compile.
        //:
        //: 6 'bslma::ManagedPtr<const T>' can be loaded with a 'T *' pointer
        //:   (cv-qualification conversion).
        //:
        //: 7 'bslma::ManagedPtr<base>' can be loaded with a 'derived *'
        //:   pointer and the deleter will destroy the 'derived' type, even if
        //:   the 'base' destructor is not virtual.
        //:
        //: 8 When 'bslma::ManagedPtr' is passed a single 'FACTORY *' argument,
        //:   the implicit deleter-function will destroy the pointed-to object
        //:   using the FACTORY::deleteObject (non-static) method.
        //:
        //: 9 'bslma::Allocator' serves as a valid FACTORY type.
        //:
        //:10 A custom type offering just the 'deleteObject' (non-virtual)
        //:   member function serves as a valid FACTORY type.
        //:
        //:11 A 'bslma::ManagedPtr' points to the same object as the pointer
        //:   passed to 'load'.  Note that this includes null pointers.
        //:
        //:12 Destroying a 'bslma::ManagedPtr' destroys any owned object using
        //:   the deleter mechanism supplied by 'load'.
        //:
        //:13 Destroying a bslma::ManagedPtr that does not own a pointer has
        //:   no observable effect.
        //:
        //:14 No 'bslma::ManagedPtr' method should allocate any memory.
        //:
        //:15 Defensive checks assert in safe build modes when passing null
        //:   pointers as arguments for factory or deleters, unless target
        //:   pointer is also null.
        //
        // Plan:
        //   take an empty pointer, and call each overload of load.
        //      confirm pointer is initially null
        //      confirm new pointer value is stored by 'ptr'
        //      confirm destructor destroys target object
        //      be sure to pass both '0' and valid pointer values to each
        //          potential overload
        //   Write a pair of nested loops
        //     For each iteration, first create a default-constructed
        //         'bslma::ManagedPtr'
        //     Then call a load function (testing each overload by the first
        //         loop)
        //     Then, in inner loop, load a second pointer and verify first
        //         target is destroyed
        //     Then verify the new target is destroyed when test object goes
        //         out of scope.
        //
        //   Test a number of scenarios in a consistent way.
        //   The 5 scenarios are:  (TestTarget)
        //      MyTestObject
        //      const MyTestObject
        //      MyDerivedObject
        //      void
        //      const void
        //
        //   For each, create a table of test-functions that use encoded names:
        //      Object  - function supplies an object to 'load'
        //      Factory - function supplies a factory to 'load'
        //      Fnull   - function passes a null pointer literal as factory
        //      Deleter - function supplies a deleter to 'load'
        //      Dzero   - function calls 'load' with a 0-value pointer variable
        //                and *not* a null-pointer literal.
        //
        //   Codes that may be passed as 'Object' policies are:
        //      Obase
        //      OCbase
        //      Oderiv
        //      OCDeriv
        //
        //   Codes for specific factories are:
        //      Fbsl   factory is cast to base 'bslma::Allocator'
        //      Ftst   factory is cast to specific 'bslma::TestAllocator' type
        //      Fdflt  factory argument is ignored and default allocator used
        //
        //   Each test-function taking an 'Object' parameter will call 'load'
        //   with both an allocate object, and a pointer variable holding a
        //   null-pointer value.
        //
        //   Each test-function taking a 'Factory' parameter will call 'load'
        //   with both an allocate object, and a pointer variable holding a
        //   null-pointer value.
        //
        //   Each of the four combinations of valid/null pointer for the
        //   factory/object arguments will be tested for test-functions taking
        //   both parameters.  The combination will be negatively tested using
        //   the 'bsls_asserttest' facility if that is appropriate.
        //
        //   These functions are assembled using further policy-functions that
        //   will create and supply objects, deleters and factories of types
        //   that are specified as template type parameters.  This allows us to
        //   compose test cases with the full set of conversion scenarios that
        //   may be needed.
        //
        //   The test function will take a default-constructed managed pointer
        //   object, call the 'load' test function, establish that the expected
        //   results of 'load' are evident, and then let the managed pointer
        //   fall out of scope, and check that the destructor destroys any
        //   managed object appropriately.
        //
        //   Then an inner-loop will again create an empty managed pointer
        //   object and 'load' it using the test function.  It will then call
        //   'load' again with the next function in the test table, and verify
        //   that the original managed object (if any) is destroyed correctly
        //   and that the new managed object is held as expected.  Then this
        //   managed pointer will fall out of scope, and we test again that any
        //   held managed object is destroyed correctly.
        //
        //   Currently well tested:
        //     const MyObjectType
        //     const void  [audit the disabled tests before moving on]
        //
        //   Note that base/derived are a conventional pair of polymorphic
        //   classes with virtual destructors.  This class should work equally
        //   well for base/derived classes that are not polymorphic, but we
        //   do not currently test that.
        //
        // Tested:
        //   void load(bsl::nullptr_t=0, bsl::nullptr_t=0, bsl::nullptr_t=0)
        //   void load(TARGET_TYPE *ptr)
        //   void load(TARGET_TYPE *ptr, FACTORY *factory)
        //   void load(ELEMENT_TYPE *ptr, void *factory, DeleterFunc deleter)
        //   void load(TARGET_TYPE *ptr,
        //             bsl::nullptr_t,
        //             void      (*deleter)(TARGET_BASE *, void*))
        //   void load(TARGET_TYPE *ptr,
        //             FACTORY *factory,
        //             void(*deleter)(TARGET_BASE*, BASE_FACTORY*))
        //   ~bslma::ManagedPtr()
        // --------------------------------------------------------------------

        if (verbose) printf("\nTesting 'load' overloads"
                            "\n------------------------\n");

        {
            if (veryVerbose) printf(
                            "Testing bslma::ManagedPtr<MyTestObject>::load\n");

            testLoadOps(L_, TEST_POLICY_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                      "Testing bslma::ManagedPtr<const MyTestObject>::load\n");

            testLoadOps(L_, TEST_POLICY_CONST_BASE_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                         "Testing bslma::ManagedPtr<MyDerivedObject>::load\n");

            testLoadOps(L_, TEST_POLICY_DERIVED_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf("Testing bslma::ManagedPtr<void>::load\n");

            testLoadOps(L_, TEST_POLICY_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                              "Testing bslma::ManagedPtr<const void>::load\n");

            testLoadOps(L_, TEST_POLICY_CONST_VOID_ARRAY);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BSLMA_MANAGEDPTR_TESTVIRTUALINHERITANCE)
        {
            if (veryVerbose) printf("Testing bslma::ManagedPtr<Base>::load\n");

            testLoadOps(L_, TEST_POLICY_BASE0_ARRAY);
        }
#endif
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        {
            if (veryVerbose) printf(
                                   "Testing bslma::ManagedPtr<Base2>::load\n");

            testLoadOps(L_, TEST_POLICY_BASE2_ARRAY);
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Compile-fail tests

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_LOAD_INCOMPATIBLE_TYPE
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_LOAD_INCOMPATIBLE_TYPE)
        {
            int i = 0;
            bslma::ManagedPtr<double> x;
            x.load(&i);

            const double d = 0.0;
            x.load(&d);

            void *v = 0;
            x.load(v);
        }
#endif

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_FACTORY
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_FACTORY)
        {
            int i = 0;
            bslma::ManagedPtr<int> x;
            x.load(&i, 0);
            x.load( 0, 0); // We may consider allowing this

            void *v = 0;
            x.load(v, 0);
            x.load(0, v); // We may consider allowing this

            bslma::Allocator * pNullAlloc = 0;
            x.load(0, pNullAlloc); // We may consider allowing this

            MyDerivedObject * pd = 0;
            bslma::ManagedPtr<MyDerivedObject> md;
            md.load(pd, 0);
        }
#endif

//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_DELETER
#if defined(BSLMA_MANAGEDPTR_COMPILE_FAIL_TEST_LOAD_NULL_DELETER)
        {
            int *i = 0;
            bslma::ManagedPtr<int> x;
            x.load(i, 0, 0);
            x.load(0, 0, 0); // We may consider allowing this

            bslma::Allocator * pNullAlloc = 0;
            x.load(i, pNullAlloc, 0);
            x.load(0, pNullAlloc, 0);  // We may consider allowing this
        }
#endif
      } break;
      case 6: {
        // --------------------------------------------------------------------
        // PRIMARY CREATORS TEST
        //   Note that we will not deem the destructor to be completely tested
        //   until the next test case, which tests the range of management
        //   strategies a bslma::ManagedPtr may hold.
        //
        // Concerns:
        //: 1 A default constructed 'bslma::ManagedPtr' does not own a pointer.
        //: 2 A default constructed 'bslma::ManagedPtr' does not allocate any
        //:   memory.
        //: 3 A 'bslma::ManagedPtr' takes ownership of a pointer passed as a
        //:   single argument to its constructor, and destroys the pointed-to
        //:   object in its destructor using the default allocator.  It does
        //:   not allocate any memory.
        //: 4 A 'bslma::ManagedPtr<base>' object created by passing a
        //:   'derived*' :   pointer calls the 'derived' destructor when
        //:   destroying the managed object, regardless of whether the 'base'
        //:   destructor is declared as 'virtual'.  No memory is allocated by
        //:   'bslma::ManagedPtr'.
        //: 5 A 'bslma::ManagedPtr<void>' object created by passing a
        //:   'derived*' pointer calls the 'derived' destructor when destroying
        //    the managed object.  No memory is allocated by
        //    'bslma::ManagedPtr'.
        //: 6 A 'bslma::ManagedPtr' taking ownership of a null pointer passed
        //:   as a single argument is equivalent to default construction; it
        //:   does not allocate any memory.
        //
        // Plan:
        //    TBD
        //
        // Tested:
        //   bslma::ManagedPtr();
        //   bslma::ManagedPtr(nullptr_t);
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING PRIMARY CREATORS"
                            "\n------------------------\n");

        using namespace CREATORS_TEST_NAMESPACE;

        if (verbose) printf("\tTest default constructor\n");

        int numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tBasic test object\n");

            bslma::TestAllocatorMonitor dam(&da);
            Obj o;

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tvoid type\n");

            bslma::TestAllocatorMonitor dam(&da);
            bslma::ManagedPtr<void> o;

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tconst-qualified int\n");

            bslma::TestAllocatorMonitor dam(&da);
            bslma::ManagedPtr<const int> o;

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTest constructing with a null pointer\n");

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tBasic test object\n");

            bslma::TestAllocatorMonitor dam(&da);
            Obj o(0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tvoid type\n");

            bslma::TestAllocatorMonitor dam(&da);
            VObj o(0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tconst-qualified int\n");

            bslma::TestAllocatorMonitor dam(&da);
            bslma::ManagedPtr<const int> o(0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTest constructing with two null pointers\n");

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tBasic test object\n");

            bslma::TestAllocatorMonitor dam(&da);
            Obj o(0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tvoid type\n");

            bslma::TestAllocatorMonitor dam(&da);
            VObj o(0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tconst-qualified int\n");

            bslma::TestAllocatorMonitor dam(&da);
            bslma::ManagedPtr<const int> o(0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\tTest constructing with three null pointers\n");

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tBasic test object\n");

            bslma::TestAllocatorMonitor dam(&da);
            Obj o(0, 0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tvoid type\n");

            bslma::TestAllocatorMonitor dam(&da);
            VObj o(0, 0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

        numDeletes = 0;
        {
            if (veryVerbose) printf("\t\tconst-qualified int\n");

            bslma::TestAllocatorMonitor dam(&da);
            bslma::ManagedPtr<const int> o(0, 0, 0);

            ASSERT(0 == o.ptr());
            ASSERT(dam.isTotalSame());
        }
        ASSERT(0 == numDeletes);

      } break;
      case 5: {
      } break;
      case 4: {
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING bslma::ManagedPtr_Ref
        //
        // 'bslma::ManagedPtr_Ref' is similar to an in-core value semantic type
        // having a single pointer as its only attribute; it does not offer the
        // traditional range of value-semantic operations such as equality
        // comparison and printing.  Its test concerns and plan are closely
        // modeled after such a value-semantic type.
        //
        // Concerns:
        //: 1 TBD Enumerate concerns
        //
        // Plan:
        //: 1 blah ...
        //
        // Testing:
        //    explicit bslma::ManagedPtr_Ref(bslma::ManagedPtr_Members *base);
        //    bslma::ManagedPtr_Ref(const bslma::ManagedPtr_Ref& original);
        //    ~bslma::ManagedPtr_Ref();
        //    bslma::ManagedPtr_Ref& operator=(const bslma::ManagedPtr_Ref&);
        //    bslma::ManagedPtr_Members *base() const;
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bslma::ManagedPtr_Ref"
                            "\n----------------------------\n");

        bslma::TestAllocatorMonitor gam(&globalAllocator);
        bslma::TestAllocatorMonitor dam(&da);

        {
            int deleteCount = 0;
            MyTestObject x(&deleteCount);

            {
                bslma::ManagedPtr_Members empty;
                bslma::ManagedPtr_Members simple(&x, 0, doNothingDeleter);

                if (verbose) printf("\tTest value constructor\n");

                const bslma::ManagedPtr_Ref<MyTestObject> ref(&empty, 0);
                bslma::ManagedPtr_Members * base = ref.base();
                LOOP2_ASSERT(&empty, base, &empty == base);

                if (verbose) printf("\tTest copy constructor\n");

                bslma::ManagedPtr_Ref<MyTestObject> other = ref;
                base = ref.base();
                LOOP2_ASSERT(&empty, base, &empty == base);
                base = other.base();
                LOOP2_ASSERT(&empty, base, &empty == base);

                if (verbose) printf("\tTest assignment\n");

                const bslma::ManagedPtr_Ref<MyTestObject> second(&simple, &x);
                base = second.base();
                LOOP2_ASSERT(&simple, base, &simple == base);


                other = second;

                base = ref.base();
                LOOP2_ASSERT(&empty, base, &empty == base);
                base = other.base();
                LOOP2_ASSERT(&simple, base, &simple == base);
                base = second.base();
                LOOP2_ASSERT(&simple, base, &simple == base);

                if (verbose) printf("\tTest destructor\n");
            }

            LOOP_ASSERT(deleteCount, 0 == deleteCount);
        }

#ifdef BDE_BUILD_TARGET_EXC
        if (verbose) printf("\tNegative testing\n");

        {
            bsls::AssertTestHandlerGuard guard;
            ASSERT_SAFE_FAIL_RAW(bslma::ManagedPtr_Ref<MyTestObject> x(0, 0));
        }
#else
        if (verbose) printf("\tNegative testing disabled due to lack of "
                             "exception support\n");
#endif

        ASSERT(dam.isInUseSame());
        ASSERT(gam.isInUseSame());
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING TEST MACHINERY
        //
        // Concerns:
        //: 1 'MyTestObject', 'MyDerivedObject' and 'MySecondDerivedObject'
        //:   objects do not allocate any memory from the default allocator nor
        //:   from the global allocator for any of their operations.
        //:
        //: 2 'MyTestObject', 'MyDerivedObject' and 'MySecondDerivedObject'
        //:   objects, created with a pointer to an integer, increment the
        //:   referenced integer exactly once when they are destroyed.
        //:
        //: 3 'MyTestObject', 'MyDerivedObject' and 'MySecondDerivedObject'
        //:   objects, created by copying another object of the same type,
        //:   increment the integer referenced by the original object, exactly
        //:   once, when they are destroyed.
        //:
        //: 4 'MyDerivedObject' is derived from 'MyTestObject'.
        //:
        //: 5 'MySecondDerivedObject' is derived from 'MyTestObject'.
        //:
        //: 6 'MyDerivedObject' is *not* derived from 'MySecondDerivedObject',
        //:   nor is 'MySecondDerivedObject' derived from 'MyDerivedObject'.
        //
        // Plan:
        //: 1 Install test allocator monitors to verify that neither the global
        //:   nor default allocators allocate any memory executing this test
        //:   case.
        //:
        //: 2 For each test-class type:
        //:   1 Initialize an 'int' counter to zero
        //:   2 Create a object of tested type, having the address of the 'int'
        //:     counter.
        //:   3 Confirm the test object 'deleterCounter' points to the 'int'
        //:     counter.
        //:   4 Confirm the 'int' counter value has not changed.
        //:   5 Destroy the test object and confirm the 'int' counter value
        //:     has incremented by exactly 1.
        //:   6 Create a second object of tested type, having the address of
        //:     the 'int' counter.
        //:   7 Create a copy of the second test object, and confirm both test
        //:     object's 'deleterCount' point to the same 'int' counter.
        //:   8 Confirm the 'int' counter value has not changed.
        //:   9 Destroy one test object, and confirm test 'int' counter is
        //:     incremented exactly once.
        //:  10 Destroy the other test object, and confirm test 'int' counter
        //:     is incremented exactly once.
        //:
        //: 3 For each test-class type:
        //:   1 Create a function overload set, where one function takes a
        //:     pointer to the test-class type and returns 'true', while the
        //:     other overload matches anything and returns 'false'.
        //:   2 Call each of the overloaded function sets with a pointer to
        //:     'int', and confirm each returns 'false'.
        //:   3 Call each of the overloaded function sets with a pointer to
        //:     an object of each of the test-class types, and confirm each
        //:     call returns 'true' only when the pointer type matches the
        //:     test-class type for that function, or points to a type publicly
        //:     derived from that test-class type.
        //:
        //: 4 Verify that no unexpected memory was allocated by inspecting the
        //:   allocator guards.
        //
        // Testing:
        //    class MyTestObject
        //    class MyDerivedObject
        //    class MySecondDerivedObject
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING TEST MACHINERY"
                            "\n----------------------\n");

        if (verbose) printf("\tTest class MyTestObject\n");

        bslma::TestAllocatorMonitor gam(&globalAllocator);
        bslma::TestAllocatorMonitor dam(&da);

        int destructorCount = 0;
        {
            MyTestObject mt(&destructorCount);
            ASSERT(&destructorCount == mt.deleteCounter());
            LOOP_ASSERT(destructorCount, 0 == destructorCount);
        }
        LOOP_ASSERT(destructorCount, 1 == destructorCount);

        destructorCount = 0;
        {
            MyTestObject mt1(&destructorCount);
            {
                MyTestObject mt2 = mt1;
                ASSERT(&destructorCount == mt1.deleteCounter());
                ASSERT(&destructorCount == mt2.deleteCounter());
                LOOP_ASSERT(destructorCount, 0 == destructorCount);
            }
            LOOP_ASSERT(destructorCount, 1 == destructorCount);
        }
        ASSERT(2 == destructorCount);

        if (verbose) printf("\tTest class MyDerivedObject\n");

        destructorCount = 0;
        {
            MyDerivedObject dt(&destructorCount);
            ASSERT(&destructorCount == dt.deleteCounter());
            LOOP_ASSERT(destructorCount, 0 == destructorCount);
        }
        ASSERT(100 == destructorCount);

        destructorCount = 0;
        {
            MyDerivedObject dt1(&destructorCount);
            {
                MyDerivedObject dt2 = dt1;
                ASSERT(&destructorCount == dt1.deleteCounter());
                ASSERT(&destructorCount == dt2.deleteCounter());
                LOOP_ASSERT(destructorCount, 0 == destructorCount);
            }
            LOOP_ASSERT(destructorCount, 100 == destructorCount);
        }
        ASSERT(200 == destructorCount);

        if (verbose) printf("\tTest class MySecondDerivedObject\n");

        destructorCount = 0;
        {
            MySecondDerivedObject st(&destructorCount);
            ASSERT(&destructorCount == st.deleteCounter());
            LOOP_ASSERT(destructorCount, 0 == destructorCount);
        }
        LOOP_ASSERT(destructorCount, 10000 == destructorCount);

        destructorCount = 0;
        {
            MySecondDerivedObject st1(&destructorCount);
            {
                MySecondDerivedObject st2 = st1;
                ASSERT(&destructorCount == st1.deleteCounter());
                ASSERT(&destructorCount == st2.deleteCounter());
                LOOP_ASSERT(destructorCount, 0 == destructorCount);
            }
            LOOP_ASSERT(destructorCount, 10000 == destructorCount);
       }
       ASSERT(20000 == destructorCount);

       if (verbose) printf("\tTest pointer conversions\n");

       struct Local {
            static bool matchBase(MyTestObject *) { return true; }
            static bool matchBase(...) { return false; }

            static bool matchDerived(MyDerivedObject *) { return true; }
            static bool matchDerived(...) { return false; }

            static bool matchSecond(MySecondDerivedObject *) { return true; }
            static bool matchSecond(...) { return false; }
        };

        {
            int badValue;
            ASSERT(!Local::matchBase(&badValue));
            ASSERT(!Local::matchDerived(&badValue));
            ASSERT(!Local::matchSecond(&badValue));
        }

        {
            MyTestObject mt(&destructorCount);
            ASSERT(Local::matchBase(&mt));
            ASSERT(!Local::matchDerived(&mt));
            ASSERT(!Local::matchSecond(&mt));
        }

        {
            MyDerivedObject dt(&destructorCount);
            ASSERT(Local::matchBase(&dt));
            ASSERT(Local::matchDerived(&dt));
            ASSERT(!Local::matchSecond(&dt));
        }

        {
            MySecondDerivedObject st(&destructorCount);
            ASSERT(Local::matchBase(&st));
            ASSERT(!Local::matchDerived(&st));
            ASSERT(Local::matchSecond(&st));
        }

        ASSERT(dam.isInUseSame());
        ASSERT(gam.isInUseSame());

      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   1. That the functions exist with the documented signatures.
        //   2. That the basic functionality works as documented.
        //
        // Plan:
        //   Exercise each function in turn and devise an elementary test
        //   sequence to ensure that the basic functionality is as documented.
        //
        // Testing:
        //   This test exercises basic functionality but *tests* *nothing*.
        // --------------------------------------------------------------------

        if (verbose) printf("\nBREATHING TEST"
                            "\n--------------\n");

        if (verbose) printf("\tTest copy construction.\n");

        bslma::TestAllocator ta("object", veryVeryVeryVerbose);

        int numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            Obj o2(o);

            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest assignment.\n");

        numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            Obj o2;

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o2  = o;

            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest construction from an rvalue.\n");

        numDeletes = 0;
        {
            bslma::TestAllocatorMonitor tam(&ta);

            Obj x(returnManagedPtr(&numDeletes, &ta)); Obj const &X = x;

            ASSERT(X.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest assignment from an rvalue.\n");

        numDeletes = 0;
        {
            Obj x; Obj const &X = x;
            x = returnManagedPtr(&numDeletes, &ta);

            ASSERT(X.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest conversion construction.\n");

        numDeletes = 0;
        {
            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            DObj o(p);

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o2(o); // conversion construction

            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            CObj o3(o2); // const-conversion construction

            ASSERT(p == o3.ptr());
            ASSERT(0 == o2.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        if (verbose) printf("\tTest conversion assignment.\n");

        numDeletes = 0;
        {
            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            DObj o(p);

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o2;
            o2  = o; // conversion assignment

            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            CObj o3;
            o3 = o2; // const-conversion assignment

            ASSERT(p == o3.ptr());
            ASSERT(0 == o2.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        if (verbose)
            printf("\tTest conversion construction from an rvalue.\n");

        numDeletes = 0;
        {
            Obj x(returnDerivedPtr(&numDeletes, &ta)); Obj const &X = x;

            ASSERT(X.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        if (verbose)
            printf("\tTest conversion assignment from an rvalue.\n");

        numDeletes = 0;
        {
            Obj x; Obj const &X = x;
            x = returnDerivedPtr(&numDeletes, &ta); // conversion-assignment
                                                    // from an rvalue

            ASSERT(X.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        if (verbose) printf("\tTest alias construction.\n");

        numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            ASSERT(0 != p);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            bslma::ManagedPtr<int> o2(o, o->valuePtr()); // alias construction

            ASSERT(p->valuePtr() == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest alias construction with conversion.\n");

        numDeletes = 0;
        {
            TDObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyDerivedObject(&numDeletes);
            ASSERT(0 != p);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            bslma::ManagedPtr<int> o2(o, o->valuePtr()); // alias construction

            ASSERT(p->valuePtr() == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 100 == numDeletes);

        if (verbose) printf("\tTest 'load' method.\n");

        numDeletes = 0;
        {
            int numDeletes2 = 0;
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes2);
            ASSERT(0 != p);
            ASSERT(0 == numDeletes2);

            Obj o(p);

            TObj *p2 = new(da) MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o.load(p2);
            ASSERT(p2 == o.ptr());
            ASSERT(1 == numDeletes2);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest 'load' method with allocator.\n");

        numDeletes = 0;
        {
            int numDeletes2 = 0;
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes2);
            ASSERT(0 == numDeletes2);

            Obj o(p);

            TObj *p2 = new(ta) MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o.load(p2,&ta);
            ASSERT(p2 == o.ptr());
            LOOP_ASSERT(numDeletes2, 1 == numDeletes2);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest 'loadAlias'.\n");

        numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            bslma::ManagedPtr<int> o2;

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o2.loadAlias(o, o->valuePtr());

            ASSERT(p->valuePtr() == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            // Check load alias to self
            o2.loadAlias(o2, p->valuePtr(1));
            ASSERT(p->valuePtr(1) == o2.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest 'swap'.\n");

        numDeletes = 0;
        {
            TObj *p = new BSLMA_IMPLICIT_ALLOCATOR MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p);
            Obj o2;

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o2.swap(o);
            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest 'swap' with custom deleter.\n");

        numDeletes = 0;
        {
            TObj *p = new(ta) MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p, &ta, &myTestDeleter);
            Obj o2;

            ASSERT(p == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            o2.swap(o);
            ASSERT(p == o2.ptr());
            ASSERT(0 == o.ptr());
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest boolean.\n");

        numDeletes = 0;
        {
            TObj *p = new(ta) MyTestObject(&numDeletes);
            LOOP_ASSERT(numDeletes, 0 == numDeletes);

            Obj o(p, &ta, &myTestDeleter);
            Obj o2;

            ASSERT(o);
            ASSERT(!o2);

            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest no-op deleter.\n");

        numDeletes = 0;
        {
            TObj x(&numDeletes);
            {
                Obj p(&x, 0, &bslma::ManagedPtrUtil::noOpDeleter);
            }
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest (deprecated) nil deleter.\n");

        numDeletes = 0;
        {
            TObj x(&numDeletes);
            {
                Obj p(&x,
                      0,
                      &bslma::ManagedPtrNilDeleter<MyTestObject>::deleter);
            }
            LOOP_ASSERT(numDeletes, 0 == numDeletes);
        }
        LOOP_ASSERT(numDeletes, 1 == numDeletes);

        if (verbose) printf("\tTest unambiguous overloads.\n");

        {
            bslma::ManagedPtr<void>      pV;
            bslma::ManagedPtr<int>       pI;
            bslma::ManagedPtr<const int> pCi;

            ASSERT(0 == OverloadTest::invoke(pV));
            ASSERT(1 == OverloadTest::invoke(pI));
            ASSERT(2 == OverloadTest::invoke(pCi));

#if 0  // compile fail test, think about giving a named macro to test
            bslma::ManagedPtr<double>    pD;
            ASSERT(0 == OverloadTest::invoke(pD));
#endif
        }
     } break;
     case -1: {
        // --------------------------------------------------------------------
        // TESTING ADDITIONAL CONCERNS
        //
        // Concerns:
        //: 1 Two 'bslma::ManagedPtr<T>' objects should not be comparable with
        //:   the equality operator.
        //
        //: 2 Two objects of different instantiations of the
        //:   'bslma::ManagedPtr' class template should not be comparable with
        //:   the equality operator.
        //
        // Plan:
        //   The absence of a specific operator will be tested by failing to
        //   compile test code using that operator.  These tests will be
        //   configured to compile only when specific macros are defined as
        //   part of the build configuration, and not routinely tested.
        //
        // Testing:
        //   This test is checking for the *absence* of the following operators
        //: o 'operator=='.
        //: o 'operator!='.
        //: o 'operator<'.
        //: o 'operator<='.
        //: o 'operator>='.
        //: o 'operator>'.
        // --------------------------------------------------------------------
//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_COMPARISON
//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_ORDERING
//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_COMPARISON
//#define BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_ORDERING

#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_COMPARISON
        {
            bslma::ManagedPtr<int> x;
            bool b;

            // The following six lines should fail to compile
            b = (x == x);
            b = (x != x);
        }
#endif

#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_HOMOGENEOUS_ORDERING
        {
            bslma::ManagedPtr<int> x;
            bool b;

            // The following six lines should fail to compile
            b = (x <  x);
            b = (x <= x);
            b = (x >= x);
            b = (x >  x);
        }
#endif

#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_COMPARISON
        {
            bslma::ManagedPtr<int>    x;
            bslma::ManagedPtr<double> y;

            bool b;

            // The following twelve lines should fail to compile
            b = (x == y);
            b = (x != y);

            b = (y == x);
            b = (y != x);
        }
#endif

#if defined BSLMA_MANAGEDPTR_COMPILE_FAIL_HETEROGENEOUS_ORDERING
        {
            bslma::ManagedPtr<int>    x;
            bslma::ManagedPtr<double> y;

            bool b;

            // The following twelve lines should fail to compile
            b = (x <  y);
            b = (x <= y);
            b = (x >= y);
            b = (x >  y);

            b = (y <  x);
            b = (y <= x);
            b = (y >= x);
            b = (y >  x);
        }
#endif
      } break;
      default: {
        fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        testStatus = -1;
      }
    }

    // CONCERN: In no case does memory come from the global allocator.

    LOOP_ASSERT(globalAllocator.numBlocksTotal(),
                0 == globalAllocator.numBlocksTotal());

    if (testStatus > 0) {
        fprintf(stderr, "Error, non-zero test status = %d.\n", testStatus);
    }
    return testStatus;
}


// ----------------------------------------------------------------------------
// Copyright (C) 2013 Bloomberg L.P.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------- END-OF-FILE ----------------------------------