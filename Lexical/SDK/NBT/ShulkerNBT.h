#pragma once

// ShulkerNBT.h
//
// NBT helpers for shulker-box container manipulation.
//
// This header layers the two convenience primitives the dupe engine relies on
// (Tag::createNative<T>() and CompoundTag::putNative()) on top of the plain
// Tag hierarchy, and exposes a handful of shulker-container accessors used by
// Dupe.cpp / TryDupe.cpp.
//
// IMPORTANT: This header must be included AFTER the definition of Tag /
// CompoundTag / CompoundTagVariant. If your Tag header is a standalone
// "#pragma once" file, include it here first.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Tag hierarchy (re-declared here so this header is self-contained).
//
// If your build already includes the Tag header via another path, guard this
// block with the same include guard or move the Tag definitions into a shared
// header. The definitions below are identical to the Tag header you attached.
// ---------------------------------------------------------------------------

#ifndef TAG_NBT_TYPES_DEFINED
#define TAG_NBT_TYPES_DEFINED

#include <map>
#include <string_view>
#include <variant>

class IDataOutput;
class IDataInput;
class PrintStream;

class Tag {
public:
    enum Type : unsigned char {
        End = 0x00,
        Byte = 0x01,
        Short = 0x02,
        Int = 0x03,
        Int64 = 0x04,
        Float = 0x05,
        Double = 0x06,
        ByteArray = 0x07,
        String = 0x08,
        List = 0x09,
        Compound = 0x0A,
        IntArray = 0x0B,
    };

    virtual ~Tag() = default;
    virtual void deleteChildren() {}
    virtual void write(IDataOutput&) const {}
    virtual void load(IDataInput&) {}
    virtual std::string toString() const { return ""; }
    virtual Tag::Type getId() const { return Type::End; }
    virtual bool equals(const Tag&) const { return false; }
    virtual void print(PrintStream&) const {}
    virtual void print(const std::string&, PrintStream&) const {}
    virtual std::unique_ptr<Tag> copy() const;
    virtual size_t hash() const { return 0; }

    // ----------------------------------------------------------------------
    // Convenience factory used by the dupe engine.
    //
    //   auto shortTag = Tag::createNative<ShortTag>(int16_t{1});
    //   auto compound = Tag::createNative<CompoundTag>();
    //
    // Defined after CompoundTag so that every concrete tag type is complete.
    // ----------------------------------------------------------------------
    template <typename T, typename... Args>
    static std::unique_ptr<T> createNative(Args&&... args);
};

class EndTag final : public Tag {
public:
    Tag::Type getId() const override { return Type::End; }
};

class ByteTag final : public Tag {
public:
    uint8_t data = 0;
    ByteTag() = default;
    explicit ByteTag(uint8_t value) : data(value) {}
    Tag::Type getId() const override { return Type::Byte; }
};

class ShortTag final : public Tag {
public:
    int16_t data = 0;
    ShortTag() = default;
    explicit ShortTag(int16_t value) : data(value) {}
    Tag::Type getId() const override { return Type::Short; }
};

class IntTag final : public Tag {
public:
    int data = 0;
    IntTag() = default;
    explicit IntTag(int value) : data(value) {}
    Tag::Type getId() const override { return Type::Int; }
};

class Int64Tag final : public Tag {
public:
    int64_t data = 0;
    Int64Tag() = default;
    explicit Int64Tag(int64_t value) : data(value) {}
    Tag::Type getId() const override { return Type::Int64; }
};

class FloatTag final : public Tag {
public:
    float data = 0.0f;
    FloatTag() = default;
    explicit FloatTag(float value) : data(value) {}
    Tag::Type getId() const override { return Type::Float; }
};

class DoubleTag final : public Tag {
public:
    double data = 0.0;
    DoubleTag() = default;
    explicit DoubleTag(double value) : data(value) {}
    Tag::Type getId() const override { return Type::Double; }
};

class ByteArrayTag final : public Tag {
public:
    std::vector<uint8_t> data;
    Tag::Type getId() const override { return Type::ByteArray; }
};

class StringTag final : public Tag {
public:
    std::string data;
    StringTag() = default;
    explicit StringTag(std::string value) : data(std::move(value)) {}
    Tag::Type getId() const override { return Type::String; }
};

class ListTag;
class CompoundTag;
class IntArrayTag;
class CompoundTagVariant;

class ListTag final : public Tag {
public:
    std::vector<std::unique_ptr<Tag>> mList;
    Tag::Type mType = Type::End;
    Tag::Type getId() const override { return Type::List; }
};

class IntArrayTag final : public Tag {
public:
    std::vector<int> data;
    Tag::Type getId() const override { return Type::IntArray; }
};

typedef std::map<std::string, CompoundTagVariant, std::less<>> TagMap;

class CompoundTag : public Tag {
public:
    TagMap mTags;
    Tag::Type getId() const override { return Type::Compound; }

    // ----------------------------------------------------------------------
    // putNative() - insert or overwrite a named child tag.
    //
    // The templated overload forwards the concrete type to the Tag overload,
    // so callers can write:
    //
    //   entry->putNative("id", Tag::createNative<ShortTag>(42));
    //   entry->putNative("tag", Tag::createNative<CompoundTag>());
    //
    // Defined after CompoundTagVariant so the variant alternatives are known.
    // ----------------------------------------------------------------------
    template <typename T>
    bool putNative(const std::string& name, std::unique_ptr<T> tag);

    bool putNative(const std::string& name, std::unique_ptr<Tag> tag);
};

class CompoundTagVariant {
    using Variant = std::variant<
        EndTag,
        ByteTag,
        ShortTag,
        IntTag,
        Int64Tag,
        FloatTag,
        DoubleTag,
        ByteArrayTag,
        StringTag,
        ListTag,
        CompoundTag,
        IntArrayTag
    >;

public:
    Variant mTagStorage;

    const Tag* get() const {
        return std::visit(
            [](const auto& tag) -> const Tag* { return &tag; },
            mTagStorage
        );
    }

    Tag* get() {
        return std::visit(
            [](auto& tag) -> Tag* { return &tag; },
            mTagStorage
        );
    }

    const Tag* operator->() const { return get(); }
    Tag* operator->() { return get(); }

    template <typename T>
    const T* get() const {
        return std::get_if<T>(&mTagStorage);
    }

    template <typename T>
    T* get() {
        return std::get_if<T>(&mTagStorage);
    }
};

// Native item loading clones tags through this virtual method. Containers
// must own independent children; returning null leaves invalid native NBT.
inline std::unique_ptr<Tag> Tag::copy() const {
    switch (getId()) {
    case End:       return std::make_unique<EndTag>();
    case Byte:      return std::make_unique<ByteTag>(*static_cast<const ByteTag*>(this));
    case Short:     return std::make_unique<ShortTag>(*static_cast<const ShortTag*>(this));
    case Int:       return std::make_unique<IntTag>(*static_cast<const IntTag*>(this));
    case Int64:     return std::make_unique<Int64Tag>(*static_cast<const Int64Tag*>(this));
    case Float:     return std::make_unique<FloatTag>(*static_cast<const FloatTag*>(this));
    case Double:    return std::make_unique<DoubleTag>(*static_cast<const DoubleTag*>(this));
    case ByteArray: return std::make_unique<ByteArrayTag>(*static_cast<const ByteArrayTag*>(this));
    case String:    return std::make_unique<StringTag>(*static_cast<const StringTag*>(this));
    case IntArray:  return std::make_unique<IntArrayTag>(*static_cast<const IntArrayTag*>(this));
    case List: {
        const auto& source = *static_cast<const ListTag*>(this);
        auto result = std::make_unique<ListTag>();
        result->mType = source.mType;
        for (const auto& child : source.mList)
            result->mList.push_back(child->copy());
        return result;
    }
    case Compound: {
        auto result = std::make_unique<CompoundTag>();
        for (const auto& [name, value] :
            static_cast<const CompoundTag*>(this)->mTags) {
            CompoundTagVariant cloned;
            std::visit(
                [&](const auto& tag) {
                    using T = std::decay_t<decltype(tag)>;
                    auto owned = tag.copy();
                    cloned.mTagStorage.emplace<T>(
                        std::move(*static_cast<T*>(owned.get()))
                    );
                },
                value.mTagStorage
            );
            result->mTags.emplace(name, std::move(cloned));
        }
        return result;
    }
    default:
        return std::make_unique<EndTag>();
    }
}

#endif // TAG_NBT_TYPES_DEFINED

// ---------------------------------------------------------------------------
// Tag::createNative<T>() definition.
// ---------------------------------------------------------------------------

template <typename T, typename... Args>
inline std::unique_ptr<T> Tag::createNative(Args&&... args) {
    static_assert(
        std::is_base_of_v<Tag, T>,
        "Tag::createNative<T> requires T to derive from Tag"
        );
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// ---------------------------------------------------------------------------
// CompoundTag::putNative() definitions.
// ---------------------------------------------------------------------------

inline bool CompoundTag::putNative(
    const std::string& name,
    std::unique_ptr<Tag> tag
) {
    if (!tag) {
        return false;
    }

    CompoundTagVariant stored;
    switch (tag->getId()) {
    case Tag::End:
        stored.mTagStorage.emplace<EndTag>(
            *static_cast<EndTag*>(tag.get())
        );
        break;
    case Tag::Byte:
        stored.mTagStorage.emplace<ByteTag>(
            *static_cast<ByteTag*>(tag.get())
        );
        break;
    case Tag::Short:
        stored.mTagStorage.emplace<ShortTag>(
            *static_cast<ShortTag*>(tag.get())
        );
        break;
    case Tag::Int:
        stored.mTagStorage.emplace<IntTag>(
            *static_cast<IntTag*>(tag.get())
        );
        break;
    case Tag::Int64:
        stored.mTagStorage.emplace<Int64Tag>(
            *static_cast<Int64Tag*>(tag.get())
        );
        break;
    case Tag::Float:
        stored.mTagStorage.emplace<FloatTag>(
            *static_cast<FloatTag*>(tag.get())
        );
        break;
    case Tag::Double:
        stored.mTagStorage.emplace<DoubleTag>(
            *static_cast<DoubleTag*>(tag.get())
        );
        break;
    case Tag::ByteArray:
        stored.mTagStorage.emplace<ByteArrayTag>(
            std::move(*static_cast<ByteArrayTag*>(tag.get()))
        );
        break;
    case Tag::String:
        stored.mTagStorage.emplace<StringTag>(
            std::move(*static_cast<StringTag*>(tag.get()))
        );
        break;
    case Tag::List:
        stored.mTagStorage.emplace<ListTag>(
            std::move(*static_cast<ListTag*>(tag.get()))
        );
        break;
    case Tag::Compound:
        stored.mTagStorage.emplace<CompoundTag>(
            std::move(*static_cast<CompoundTag*>(tag.get()))
        );
        break;
    case Tag::IntArray:
        stored.mTagStorage.emplace<IntArrayTag>(
            std::move(*static_cast<IntArrayTag*>(tag.get()))
        );
        break;
    default:
        return false;
    }

    mTags.insert_or_assign(name, std::move(stored));
    return true;
}

template <typename T>
inline bool CompoundTag::putNative(
    const std::string& name,
    std::unique_ptr<T> tag
) {
    static_assert(
        std::is_base_of_v<Tag, T>,
        "CompoundTag::putNative<T> requires T to derive from Tag"
        );
    std::unique_ptr<Tag> base = std::move(tag);
    return putNative(name, std::move(base));
}

// ---------------------------------------------------------------------------
// Native game bindings.
//
// Dupe.cpp / TryDupe.cpp call into native Bedrock code to serialize and
// deserialize ItemStacks. These are thin wrappers so the rest of the codebase
// does not have to include Addresses.h directly.
// ---------------------------------------------------------------------------

namespace ShulkerNBT {

    // Serialize a native ItemStack into a fresh CompoundTag entry using the
    // game's ItemStackBase::save. Returns null if the save function or the
    // stack is unavailable.
    //
    // Implementation lives in Dupe.cpp; this declaration is here so callers
    // (including TryDupe.cpp) can link against it without pulling in the full
    // Dupe class.
    std::unique_ptr<CompoundTag> saveNativeItemEntry(const ItemStack& stack);

    // Deserialize a CompoundTag entry into a native ItemStack using the game's
    // ItemStack::loadItem. Declared here for symmetry with saveNativeItemEntry.
    void loadNativeItemEntry(ItemStack& out, const CompoundTag& entry);

    // Find the shulker "Items" list on a root compound, checking the common
    // Bedrock spellings ("Items", "items", "__Legacy__Items") and descending
    // through "tag"/"BlockEntityTag" wrappers. Returns null if not found.
    const ListTag* findShulkerItemsList(const CompoundTag* root);

    // Mutable overload of findShulkerItemsList.
    ListTag* findMutableShulkerItemsList(CompoundTag* root);

} // namespace ShulkerNBT