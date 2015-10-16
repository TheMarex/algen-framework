#include "catch.hpp"

//#include "../pq/nonaddressable_pairing_heap.hpp"
#include "../pq/addressable_pairing_heap.hpp"

using TestHeap = addressable_pairing_heap<unsigned, std::greater<unsigned>>;
using TestHeapInt = addressable_pairing_heap<int, std::greater<int>>;

struct ComplexTestKey
{
    ComplexTestKey() = default;
    ComplexTestKey(unsigned a, unsigned b) : a(a), b(b) {}
    unsigned a;
    unsigned b;
};

std::ostream& operator<<(std::ostream& lhs, const ComplexTestKey& rhs)
{
    lhs << "[" << rhs.a << "," << rhs.b << "]" << std::endl;
    return lhs;
}

bool operator==(const ComplexTestKey& lhs, const ComplexTestKey& rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b;
}

bool operator<(const ComplexTestKey& lhs, const ComplexTestKey& rhs)
{
    return lhs.a < rhs.a || (lhs.a == rhs.a && lhs.b < rhs.b);
}

bool operator>(const ComplexTestKey& lhs, const ComplexTestKey& rhs)
{
    return lhs.a > rhs.a || (lhs.a == rhs.a && lhs.b > rhs.b);
}

using ComplexTestHeap = addressable_pairing_heap<ComplexTestKey, std::greater<ComplexTestKey>>;

SCENARIO("pairing_heap's basic functions work", "[pairing_heap]")
{
    GIVEN("An empty heap")
    {
        TestHeap pq;

        WHEN("We ask for the size")
        {
            THEN("The size is 0")
            {
                CHECK(pq.size() == 0);
            }
            pq.push(1);
            AND_THEN("After inserting one element the size is 1")
            {
                CHECK(pq.size() == 1);
            }
            pq.pop();
            AND_THEN("After removing one element the size is 0 again")
            {
                CHECK(pq.size() == 0);
            }
        }

        WHEN("We ask for the top element")
        {
            THEN("It throws.")
            {
                //CHECK_THROWS(pq.top());
            }
            pq.push(5);
            AND_THEN("After inserting one element it returns it")
            {
                CHECK(pq.top() == 5);
            }
        }
        WHEN("We emplace a list of elements")
        {
            pq.emplace(1337);
            THEN("They are added.")
            {
                CHECK(pq.size() == 1);
                CHECK(pq.top() == 1337);
            }
        }

        WHEN("We do (push-pop-push)^6")
        {
            pq.push(1);
            pq.pop();
            pq.push(5);

            pq.push(1);
            pq.pop();
            pq.push(7);

            pq.push(1);
            pq.pop();
            pq.push(12);

            pq.push(1);
            pq.pop();
            pq.push(1337);

            pq.push(1);
            pq.pop();
            pq.push(1);

            pq.push(1);
            pq.pop();
            pq.push(3);

            THEN("size is correct")
            {
                CHECK(pq.size() == 6);
            }
        }

        WHEN("We do (push-pop-push)^6 (pop-push-pop)^6")
        {
            pq.push(1);
            pq.pop();
            pq.push(5);

            pq.push(1);
            pq.pop();
            pq.push(7);

            pq.push(1);
            pq.pop();
            pq.push(12);

            pq.push(1);
            pq.pop();
            pq.push(1337);

            pq.push(1);
            pq.pop();
            pq.push(1);

            pq.push(1);
            pq.pop();
            pq.push(3);

            pq.pop();
            pq.push(5);
            pq.pop();

            pq.pop();
            pq.push(7);
            pq.pop();

            pq.pop();
            pq.push(12);
            pq.pop();

            pq.pop();
            pq.push(1337);
            pq.pop();

            pq.pop();
            pq.push(1);
            pq.pop();

            pq.pop();
            pq.push(3);
            pq.pop();

            THEN("size is correct")
            {
                CHECK(pq.size() == 0);
            }
        }

        WHEN("We increase the key of the top")
        {
            // (5), (7), (12), (1337), (1), (3)
            // pop -> (5: (7)), (12: (1337)), (3)
            // pop -> (5: (7), (12: 1337))
            auto* elem = pq.push(5);
            pq.push(7);
            pq.push(12);
            pq.push(1337);
            pq.push(1);
            pq.push(3);

            pq.pop();
            pq.pop();

            THEN("The top after pops is correct")
            {
                CHECK(pq.top() == 5);
            }

            pq.modify(elem, 1338);

            THEN("The top after modify is correct")
            {
                CHECK(pq.top() == 7);
            }
        }

        WHEN("We increase the key of the an element with children")
        {
            // (5), (7), (12), (1337), (1), (3)
            // pop -> (5: (7)), (12: (1337)), (3)
            // pop -> (5: (7), (12: 1337))
            pq.push(5);
            pq.push(7);
            auto* elem = pq.push(12);
            pq.push(1337);
            pq.push(1);
            pq.push(3);

            pq.pop();
            pq.pop();

            THEN("The top after pops is correct")
            {
                CHECK(pq.top() == 5);
            }

            pq.modify(elem, 1338);

            THEN("The top did not change")
            {
                CHECK(pq.top() == 5);
            }
        }

    }

    GIVEN("An heap with six elements")
    {
        TestHeap pq;
        pq.push(5);
        pq.push(7);
        pq.push(12);
        pq.push(1337);
        pq.push(1);
        pq.push(3);

        WHEN("We ask for the top element")
        {
            THEN("It returns the min and has correct size.")
            {
                CHECK(pq.top() == 1);
                CHECK(pq.size() == 6);
            }
        }

        WHEN("We remove the top element")
        {
            pq.pop();
            THEN("It returns the new min and has correct size.")
            {
                CHECK(pq.top() == 3);
                CHECK(pq.size() == 5);
            }
        }

        WHEN("We remove the first 5 elements")
        {
            pq.pop();
            pq.pop();
            pq.pop();
            pq.pop();
            pq.pop();
            THEN("It returns the lowest element and size 1.")
            {
                CHECK(pq.size() == 1);
                CHECK(pq.top() == 1337);
            }
        }

        WHEN("We remove all elements")
        {
            pq.pop();
            pq.pop();
            pq.pop();
            pq.pop();
            pq.pop();
            pq.pop();
            THEN("It has size 0")
            {
                CHECK(pq.size() == 0);
            }

        }

        WHEN("We increase the key of the smallest element to be the second largest")
        {
            auto* elem = pq.push(0);
            pq.modify(elem, 2);
            THEN("The top did not change")
            {
                CHECK(pq.top() == 1);
            }
            pq.pop();
            AND_THEN("It is returned as second largest")
            {
                CHECK(pq.top() == 2);
            }
        }

        WHEN("We decrease the key of the largest element to be the second smallest")
        {
            auto* elem = pq.push(9999);
            pq.modify_up(elem, 2);
            THEN("The top did not change")
            {
                CHECK(pq.top() == 1);
            }
            pq.pop();
            AND_THEN("It is returned as second largest")
            {
                CHECK(pq.top() == 2);
            }
        }

        WHEN("We remove 3 elements and decreae the lowest")
        {
            auto* elem = pq.push(9999);
            pq.pop();
            pq.pop();
            pq.pop();
            pq.modify_up(elem, 2);
            THEN("The top is the new min")
            {
                CHECK(pq.top() == 2);
            }
            pq.pop();
            AND_THEN("The new second largest is correct")
            {
                CHECK(pq.top() == 7);
            }
        }
    }
}

SCENARIO("pairing_heap work with complex keys", "[pairing_heap]")
{
    ComplexTestHeap pq;

    GIVEN("An initialized pq")
    {
        pq.emplace(0, 1);
        pq.emplace(1337, 1);
        pq.emplace(0, 2);
        pq.emplace(5, 8);

        WHEN("We ask for the top")
        {
            CHECK(pq.top() == (ComplexTestKey {0, 1}));
        }
    }
}

SCENARIO("regression tests", "[pairing_heap]")
{
    TestHeap pq;
    TestHeapInt pq_signed;

    GIVEN("The push-descrease benchmark seed")
    {
        pq.push(0);
        pq.push(1);
        pq.push(2);
        auto h1 = pq.push(222971131);
        auto h2 = pq.push(3513867340);
        auto h3 = pq.push(1581535540);
        auto h4 = pq.push(478793679);

        WHEN("We pop the first three elements")
        {
            pq.pop();
            pq.pop();
            pq.pop();

            THEN("min and size is correct")
            {
                CHECK(pq.top() == 222971131);
                CHECK(pq.size() == 4);
            }
        }

        WHEN("We pop and decrease")
        {
            pq.pop();
            pq.pop();
            pq.pop();
            pq.modify_up(h1, 222971128);
            pq.modify_up(h2, 3513867337);
            pq.modify_up(h3, 1581535537);
            pq.modify_up(h4, 478793676);

            THEN("min and size are correct")
            {
                CHECK(pq.top() == 222971128);
                CHECK(pq.size() == 4);
            }
        }
    }

    GIVEN("The random test seed from the microbenchmark")
    {
        pq_signed.push(222971128);
        pq_signed.push(-781099959);
        pq_signed.push(1581535537);
        pq_signed.push(478793676);
        pq_signed.push(244574117);
        pq_signed.push(1677044595);
        pq_signed.push(2035291173);
        pq_signed.push(766503359);

        WHEN("We ask for the top element")
        {
            CHECK(pq_signed.size() == 8);
            CHECK(pq_signed.top() == -781099959);
        }

        WHEN("We remove all elements")
        {
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();
            pq_signed.pop();

            CHECK(pq.size() == 0);
        }
    }
}

// This class uses features from GNU libstdc++'s policy-base
// data structures library
#if defined(__GNUG__) && !(defined(__APPLE_CC__))

#include <ext/pb_ds/priority_queue.hpp>

SCENARIO("Comparing to gnu_pq", "[gnu_pq]")
{
    using our_pairing_heap = addressable_pairing_heap<unsigned, std::less<unsigned>>;
    using gnu_pairing_heap = __gnu_pbds::priority_queue<unsigned, std::less<unsigned>, __gnu_pbds::pairing_heap_tag>;

    our_pairing_heap our_pq;
    gnu_pairing_heap gnu_pq;

    GIVEN("The same elements in the same order")
    {
        our_pq.push(5);
        our_pq.push(7);
        our_pq.push(12);
        our_pq.push(1337);
        our_pq.push(1);
        our_pq.push(3);

        gnu_pq.push(5);
        gnu_pq.push(7);
        gnu_pq.push(12);
        gnu_pq.push(1337);
        gnu_pq.push(1);
        gnu_pq.push(3);

        WHEN("We check the top elemnts")
        {
            THEN("They are the same")
            {
                CHECK(gnu_pq.top() == 1337);
                CHECK(our_pq.top() == 1337);
            }
        }

        WHEN("We we remove the top 3 elements")
        {
            our_pq.pop();
            our_pq.pop();
            our_pq.pop();

            gnu_pq.pop();
            gnu_pq.pop();
            gnu_pq.pop();
            THEN("They top elements are the same")
            {
                CHECK(gnu_pq.top() == our_pq.top());
            }
        }
    }
}

#endif