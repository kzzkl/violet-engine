#include "tree_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
tree_page::tree_page()
{
    m_title = std::make_unique<text_title_1>("Tree");
    m_title->link(this);

    m_description = std::make_unique<text_content>(
        "A tree control is a vertical list containing multiple nodes, each of which can be "
        "collapsed or expanded.");
    m_description->link(this);

    m_display = std::make_unique<display_panel>();
    m_display->link(this);
    initialize_sample_tree();
}

void tree_page::initialize_sample_tree()
{
    auto& font = system<ui::ui>().font("content");

    m_tree = std::make_unique<ui::tree>();
    m_tree->link(m_display.get());

    const std::vector<std::string> chapters = {
        "Chapter 1: Accustoming Yourself to C++",
        "Chapter 2: Constructors, Destructors, and Assignment Operators",
        "Chapter 3: Resource Management",
        "Chapter 4: Designs and Declarations",
        "Chapter 5: Implementations",
        "Chapter 6: Inheritance and Object-Oriented Design",
        "Chapter 7: Templates and Generic Programming",
        "Chapter 8: Customizing new and delete",
        "Chapter 9: Miscellany"};
    const std::vector<std::string> items = {
        "Item 1: View C++ as a federation of languages.",
        "Item 2: Prefer consts, enums, and inlines to #defines.",
        "Item 3: Use const whenever possible.",
        "Item 4: Make sure that objects are initialized before they're used.",
        "Item 5: Know what functions C++ silently writes and calls.",
        "Item 6: Explicitly disallow the use of compilergenerated functions you do not want.",
        "Item 7: Declare destructors virtual in polymorphic base classes.",
        "Item 8: Prevent exceptions from leaving destructors.",
        "Item 9: Never call virtual functions during construction or destruction.",
        "Item 10: Have assignment operators return a reference to *this.",
        "Item 11: Handle assignment to self in operator=.",
        "Item 12: Copy all parts of an object.",
        "Item 13: Use objects to manage resources.",
        "Item 14: Think carefully about copying behavior in resource-managing classes.",
        "Item 15: Provide access to raw resources in resourcemanaging classes.",
        "Item 16: Use the same form in corresponding uses of new and delete.",
        "Item 17: Store newed objects in smart pointers in standalone statements.",
        "Item 18: Make interfaces easy to use correctly and hard to use incorrectly.",
        "Item 19: Treat class design as type design.",
        "Item 20: Prefer pass-by-reference-to-const to pass-by value.",
        "Item 21: Don't try to return a reference when you must return an object.",
        "Item 22: Declare data members private.",
        "Item 23: Prefer non-member non-friend functions to member functions.",
        "Item 24: Declare non-member functions when type conversions should apply to all "
        "parameters.",
        "Item 25: Consider support for a non-throwing swap.",
        "Item 26: Postpone variable definitions as long as possible.",
        "Item 27: Minimize casting.",
        "Item 28: Avoid returning \"handles\" to object internals.",
        "Item 29: Strive for exception-safe code.",
        "Item 30: Understand the ins and outs of inlining.",
        "Item 31: Minimize compilation dependencies between files.",
        "Item 32: Make sure public inheritance models \"is-a.\"",
        "Item 33: Avoid hiding inherited names.",
        "Item 34: Differentiate between inheritance of interface and inheritance of "
        "implementation.",
        "Item 35: Consider alternatives to virtual functions.",
        "Item 36: Never redefine an inherited non-virtual function.",
        "Item 37: Never redefine a function's inherited default parameter value.",
        "Item 38: Model \"has-a\" or \"is-implemented-in-termsof\" through composition.",
        "Item 39: Use private inheritance judiciously.",
        "Item 40: Use multiple inheritance judiciously.",
        "Item 41: Understand implicit interfaces and compiletime polymorphism.",
        "Item 42: Understand the two meanings of typename.",
        "Item 43: Know how to access names in templatized base classes.",
        "Item 44: Factor parameter-independent code out of templates.",
        "Item 45: Use member function templates to accept \"all compatible types.\"",
        "Item 46: Define non-member functions inside templates when type conversions are desired.",
        "Item 47: Use traits classes for information about types.",
        "Item 48: Be aware of template metaprogramming.",
        "Item 49: Understand the behavior of the new-handler.",
        "Item 50: Understand when it makes sense to replace new and delete.",
        "Item 51: Adhere to convention when writing new and delete.",
        "Item 52: Write placement delete if you write placement new.",
        "Item 53: Pay attention to compiler warnings.",
        "Item 54: Familiarize yourself with the standard library, including TR1.",
        "Item 55: Familiarize yourself with Boost."};

    for (auto& chapter : chapters)
    {
        m_chapters.push_back(std::make_unique<ui::tree_node>(chapter, font));
        m_tree->add(m_chapters.back().get());
    }

    for (auto& item : items)
        m_items.push_back(std::make_unique<ui::tree_node>(item, font));

    std::size_t i = 0;
    for (; i < 4; ++i)
        m_chapters[0]->add(m_items[i].get());
    for (; i < 12; ++i)
        m_chapters[1]->add(m_items[i].get());
    for (; i < 17; ++i)
        m_chapters[2]->add(m_items[i].get());
    for (; i < 25; ++i)
        m_chapters[3]->add(m_items[i].get());
    for (; i < 31; ++i)
        m_chapters[4]->add(m_items[i].get());
    for (; i < 40; ++i)
        m_chapters[5]->add(m_items[i].get());
    for (; i < 48; ++i)
        m_chapters[6]->add(m_items[i].get());
    for (; i < 52; ++i)
        m_chapters[7]->add(m_items[i].get());
    for (; i < 55; ++i)
        m_chapters[8]->add(m_items[i].get());
}
} // namespace ash::sample