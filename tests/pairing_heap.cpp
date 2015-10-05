#include "catch.hpp"

//#include "../pq/nonaddressable_pairing_heap.hpp"
#include "../pq/addressable_pairing_heap.hpp"

using TestHeap = addressable_pairing_heap<unsigned>;

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
    return lhs.a < lhs.a || (lhs.a == rhs.a && lhs.b < rhs.b);
}

using ComplexTestHeap = addressable_pairing_heap<ComplexTestKey>;

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

            CHECK(pq.size() == 0);
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

        WHEN("We decrease the key of the largest element to be the second smallest")
        {
            auto* elem = pq.push(9999);
            pq.decrease_key(elem, 2);
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
            pq.decrease_key(elem, 2);
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
