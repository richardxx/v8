// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_JSON_STRINGIFIER_H_
#define V8_JSON_STRINGIFIER_H_

#include "v8.h"
#include "v8utils.h"
#include "v8conversions.h"

namespace v8 {
namespace internal {

class BasicJsonStringifier BASE_EMBEDDED {
 public:
  explicit BasicJsonStringifier(Isolate* isolate);

  MaybeObject* Stringify(Handle<Object> object);

 private:
  static const int kInitialPartLength = 32;
  static const int kMaxPartLength = 16 * 1024;
  static const int kPartLengthGrowthFactor = 2;

  enum Result { UNCHANGED, SUCCESS, EXCEPTION, CIRCULAR, STACK_OVERFLOW };

  template <bool is_ascii> void Extend();

  void ChangeEncoding();

  void ShrinkCurrentPart();

  template <bool is_ascii, typename Char>
  INLINE(void Append_(Char c));

  template <bool is_ascii, typename Char>
  INLINE(void Append_(const Char* chars));

  INLINE(void Append(char c)) {
    if (is_ascii_) {
      Append_<true>(c);
    } else {
      Append_<false>(c);
    }
  }

  INLINE(void Append(const char* chars)) {
    if (is_ascii_) {
      Append_<true>(chars);
    } else {
      Append_<false>(chars);
    }
  }

  Handle<Object> GetProperty(Handle<JSObject> object,
                             Handle<String> key);

  Handle<Object> ApplyToJsonFunction(Handle<Object> object,
                                     Handle<Object> key);

  // Entry point to serialize the object.
  INLINE(Result SerializeObject(Handle<Object> obj)) {
    return Serialize_<false>(obj, false, isolate_->factory()->empty_string());
  }

  // Serialize an array element.
  // The index may serve as argument for the toJSON function.
  INLINE(Result SerializeElement(Handle<Object> object, int i)) {
    return Serialize_<false>(object, false, Handle<Object>(Smi::FromInt(i)));
  }

  // Serialize a object property.
  // The key may or may not be serialized depending on the property.
  // The key may also serve as argument for the toJSON function.
  INLINE(Result SerializeProperty(Handle<Object> object,
                                  bool deferred_comma,
                                  Handle<String> deferred_key)) {
    ASSERT(!deferred_key.is_null());
    return Serialize_<true>(object, deferred_comma, deferred_key);
  }

  template <bool deferred_string_key>
  Result Serialize_(Handle<Object> object, bool comma, Handle<Object> key);

  void SerializeDeferredKey(bool deferred_comma, Handle<Object> deferred_key) {
    if (deferred_comma) Append(',');
    SerializeString(Handle<String>::cast(deferred_key));
    Append(':');
  }

  Result SerializeSmi(Smi* object);

  Result SerializeDouble(double number);
  INLINE(Result SerializeHeapNumber(Handle<HeapNumber> object)) {
    return SerializeDouble(object->value());
  }

  Result SerializeJSValue(Handle<JSValue> object);

  INLINE(Result SerializeJSArray(Handle<JSArray> object));
  INLINE(Result SerializeJSObject(Handle<JSObject> object));

  Result SerializeJSArraySlow(Handle<JSArray> object, int length);

  void SerializeString(Handle<String> object);

  template <typename SrcChar, typename DestChar>
  INLINE(void SerializeStringUnchecked_(const SrcChar* src,
                                        DestChar* dest,
                                        int length));

  template <bool is_ascii, typename Char>
  INLINE(void SerializeString_(Vector<const Char> vector,
                               Handle<String> string));

  template <typename Char>
  INLINE(bool DoNotEscape(Char c));

  template <typename Char>
  INLINE(Vector<const Char> GetCharVector(Handle<String> string));

  Result StackPush(Handle<Object> object);
  void StackPop();

  INLINE(Handle<String> accumulator()) {
    return Handle<String>(String::cast(accumulator_store_->value()));
  }

  INLINE(void set_accumulator(Handle<String> string)) {
    return accumulator_store_->set_value(*string);
  }

  Isolate* isolate_;
  // We use a value wrapper for the string accumulator to keep the
  // (indirect) handle to it in the outermost handle scope.
  Handle<JSValue> accumulator_store_;
  Handle<String> current_part_;
  Handle<String> tojson_symbol_;
  Handle<JSArray> stack_;
  int current_index_;
  int part_length_;
  bool is_ascii_;

  static const int kJsonEscapeTableEntrySize = 8;
  static const char* const JsonEscapeTable;
};


// Translation table to escape ASCII characters.
// Table entries start at a multiple of 8 and are null-terminated.
const char* const BasicJsonStringifier::JsonEscapeTable =
    "\\u0000\0 \\u0001\0 \\u0002\0 \\u0003\0 "
    "\\u0004\0 \\u0005\0 \\u0006\0 \\u0007\0 "
    "\\b\0     \\t\0     \\n\0     \\u000b\0 "
    "\\f\0     \\r\0     \\u000e\0 \\u000f\0 "
    "\\u0010\0 \\u0011\0 \\u0012\0 \\u0013\0 "
    "\\u0014\0 \\u0015\0 \\u0016\0 \\u0017\0 "
    "\\u0018\0 \\u0019\0 \\u001a\0 \\u001b\0 "
    "\\u001c\0 \\u001d\0 \\u001e\0 \\u001f\0 "
    " \0      !\0      \\\"\0     #\0      "
    "$\0      %\0      &\0      '\0      "
    "(\0      )\0      *\0      +\0      "
    ",\0      -\0      .\0      /\0      "
    "0\0      1\0      2\0      3\0      "
    "4\0      5\0      6\0      7\0      "
    "8\0      9\0      :\0      ;\0      "
    "<\0      =\0      >\0      ?\0      "
    "@\0      A\0      B\0      C\0      "
    "D\0      E\0      F\0      G\0      "
    "H\0      I\0      J\0      K\0      "
    "L\0      M\0      N\0      O\0      "
    "P\0      Q\0      R\0      S\0      "
    "T\0      U\0      V\0      W\0      "
    "X\0      Y\0      Z\0      [\0      "
    "\\\\\0     ]\0      ^\0      _\0      "
    "`\0      a\0      b\0      c\0      "
    "d\0      e\0      f\0      g\0      "
    "h\0      i\0      j\0      k\0      "
    "l\0      m\0      n\0      o\0      "
    "p\0      q\0      r\0      s\0      "
    "t\0      u\0      v\0      w\0      "
    "x\0      y\0      z\0      {\0      "
    "|\0      }\0      ~\0      \177\0      ";


BasicJsonStringifier::BasicJsonStringifier(Isolate* isolate)
    : isolate_(isolate), current_index_(0), is_ascii_(true) {
  accumulator_store_ = Handle<JSValue>::cast(
      isolate_->factory()->ToObject(isolate_->factory()->empty_string()));
  part_length_ = kInitialPartLength;
  current_part_ =
      isolate_->factory()->NewRawAsciiString(kInitialPartLength);
  tojson_symbol_ = isolate_->factory()->LookupAsciiSymbol("toJSON");
  stack_ = isolate_->factory()->NewJSArray(8);
}


MaybeObject* BasicJsonStringifier::Stringify(Handle<Object> object) {
  switch (SerializeObject(object)) {
    case UNCHANGED:
      return isolate_->heap()->undefined_value();
    case SUCCESS:
      ShrinkCurrentPart();
      return *isolate_->factory()->NewConsString(accumulator(), current_part_);
    case CIRCULAR:
      return isolate_->Throw(*isolate_->factory()->NewTypeError(
                 "circular_structure", HandleVector<Object>(NULL, 0)));
    case STACK_OVERFLOW:
      return isolate_->StackOverflow();
    default:
      return Failure::Exception();
  }
}


template <bool is_ascii, typename Char>
void BasicJsonStringifier::Append_(Char c) {
  if (is_ascii) {
    SeqAsciiString::cast(*current_part_)->SeqAsciiStringSet(
        current_index_++, c);
  } else {
    SeqTwoByteString::cast(*current_part_)->SeqTwoByteStringSet(
        current_index_++, c);
  }
  if (current_index_ == part_length_) Extend<is_ascii>();
}


template <bool is_ascii, typename Char>
void BasicJsonStringifier::Append_(const Char* chars) {
  for ( ; *chars != '\0'; chars++) Append_<is_ascii, Char>(*chars);
}


Handle<Object> BasicJsonStringifier::GetProperty(Handle<JSObject> object,
                                                 Handle<String> key) {
  LookupResult lookup(isolate_);
  object->LocalLookupRealNamedProperty(*key, &lookup);
  if (!lookup.IsProperty()) return isolate_->factory()->undefined_value();
  switch (lookup.type()) {
    case NORMAL: {
      Object* value = lookup.holder()->GetNormalizedProperty(&lookup);
      ASSERT(!value->IsTheHole());
      return Handle<Object>(value);
    }
    case FIELD: {
      Object* value = lookup.holder()->FastPropertyAt(lookup.GetFieldIndex());
      ASSERT(!value->IsTheHole());
      return Handle<Object>(value);
    }
    case CONSTANT_FUNCTION:
      return Handle<Object>(lookup.GetConstantFunction());
    default: {
      PropertyAttributes attr;
      return Object::GetProperty(object, object, &lookup, key, &attr);
    }
  }
  return Handle<Object>::null();
}


Handle<Object> BasicJsonStringifier::ApplyToJsonFunction(
    Handle<Object> object, Handle<Object> key) {
  LookupResult lookup(isolate_);
  JSObject::cast(*object)->LookupRealNamedProperty(*tojson_symbol_, &lookup);
  if (!lookup.IsProperty()) return object;
  PropertyAttributes attr;
  Handle<Object> fun =
      Object::GetProperty(object, object, &lookup, tojson_symbol_, &attr);
  if (!fun->IsJSFunction()) return object;

  // Call toJSON function.
  if (key->IsSmi()) key = isolate_->factory()->NumberToString(key);
  Handle<Object> argv[] = { key };
  bool has_exception = false;
  HandleScope scope(isolate_);
  object = Execution::Call(fun, object, 1, argv, &has_exception);
  // Return empty handle to signal an exception.
  if (has_exception) return Handle<Object>::null();
  return scope.CloseAndEscape(object);
}


BasicJsonStringifier::Result BasicJsonStringifier::StackPush(
    Handle<Object> object) {
  StackLimitCheck check(isolate_);
  if (check.HasOverflowed()) return STACK_OVERFLOW;

  int length = Smi::cast(stack_->length())->value();
  FixedArray* elements = FixedArray::cast(stack_->elements());
  for (int i = 0; i < length; i++) {
    if (elements->get(i) == *object) {
      return CIRCULAR;
    }
  }
  stack_->EnsureSize(length + 1);
  FixedArray::cast(stack_->elements())->set(length, *object);
  stack_->set_length(Smi::FromInt(length + 1));
  return SUCCESS;
}


void BasicJsonStringifier::StackPop() {
  int length = Smi::cast(stack_->length())->value();
  stack_->set_length(Smi::FromInt(length - 1));
}


template <bool deferred_string_key>
BasicJsonStringifier::Result BasicJsonStringifier::Serialize_(
    Handle<Object> object, bool comma, Handle<Object> key) {
  if (object->IsJSObject()) {
    object = ApplyToJsonFunction(object, key);
    if (object.is_null()) return EXCEPTION;
  }

  if (object->IsJSObject()) {
    if (object->IsJSFunction()) return UNCHANGED;
    if (deferred_string_key) SerializeDeferredKey(comma, key);
    if (object->IsJSArray()) {
      return SerializeJSArray(Handle<JSArray>::cast(object));
    } else if (object->IsJSValue()) {
      return SerializeJSValue(Handle<JSValue>::cast(object));
    } else {
      return SerializeJSObject(Handle<JSObject>::cast(object));
    }
  }

  // Handle non-JSObject.
  if (object->IsString()) {
    if (deferred_string_key) SerializeDeferredKey(comma, key);
    SerializeString(Handle<String>::cast(object));
    return SUCCESS;
  } else if (object->IsSmi()) {
    if (deferred_string_key) SerializeDeferredKey(comma, key);
    return SerializeSmi(Smi::cast(*object));
  } else if (object->IsHeapNumber()) {
    if (deferred_string_key) SerializeDeferredKey(comma, key);
    return SerializeHeapNumber(Handle<HeapNumber>::cast(object));
  } else if (object->IsOddball()) {
    switch (Oddball::cast(*object)->kind()) {
      case Oddball::kFalse:
        if (deferred_string_key) SerializeDeferredKey(comma, key);
        Append("false");
        return SUCCESS;
      case Oddball::kTrue:
        if (deferred_string_key) SerializeDeferredKey(comma, key);
        Append("true");
        return SUCCESS;
      case Oddball::kNull:
        if (deferred_string_key) SerializeDeferredKey(comma, key);
        Append("null");
        return SUCCESS;
    }
  }

  return UNCHANGED;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeJSValue(
    Handle<JSValue> object) {
  bool has_exception = false;
  String* class_name = object->class_name();
  if (class_name == isolate_->heap()->String_symbol()) {
    Handle<Object> value = Execution::ToString(object, &has_exception);
    if (has_exception) return EXCEPTION;
    SerializeString(Handle<String>::cast(value));
  } else if (class_name == isolate_->heap()->Number_symbol()) {
    Handle<Object> value = Execution::ToNumber(object, &has_exception);
    if (has_exception) return EXCEPTION;
    if (value->IsSmi()) return SerializeSmi(Smi::cast(*value));
    SerializeHeapNumber(Handle<HeapNumber>::cast(value));
  } else {
    ASSERT(class_name == isolate_->heap()->Boolean_symbol());
    Object* value = JSValue::cast(*object)->value();
    ASSERT(value->IsBoolean());
    Append(value->IsTrue() ? "true" : "false");
  }
  return SUCCESS;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeSmi(Smi* object) {
  static const int kBufferSize = 100;
  char chars[kBufferSize];
  Vector<char> buffer(chars, kBufferSize);
  Append(IntToCString(object->value(), buffer));
  return SUCCESS;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeDouble(
    double number) {
  if (isinf(number) || isnan(number)) {
    Append("null");
    return SUCCESS;
  }
  static const int kBufferSize = 100;
  char chars[kBufferSize];
  Vector<char> buffer(chars, kBufferSize);
  Append(DoubleToCString(number, buffer));
  return SUCCESS;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeJSArray(
    Handle<JSArray> object) {
  HandleScope handle_scope(isolate_);
  Result stack_push = StackPush(object);
  if (stack_push != SUCCESS) return stack_push;
  int length = Smi::cast(object->length())->value();
  Append('[');
  switch (object->GetElementsKind()) {
    case FAST_SMI_ELEMENTS: {
      Handle<FixedArray> elements(FixedArray::cast(object->elements()));
      for (int i = 0; i < length; i++) {
        if (i > 0) Append(',');
        SerializeSmi(Smi::cast(elements->get(i)));
      }
      break;
    }
    case FAST_DOUBLE_ELEMENTS: {
      Handle<FixedDoubleArray> elements(
          FixedDoubleArray::cast(object->elements()));
      for (int i = 0; i < length; i++) {
        if (i > 0) Append(',');
        SerializeDouble(elements->get_scalar(i));
      }
      break;
    }
    case FAST_ELEMENTS: {
      Handle<FixedArray> elements(FixedArray::cast(object->elements()));
      for (int i = 0; i < length; i++) {
        if (i > 0) Append(',');
        Result result = SerializeElement(Handle<Object>(elements->get(i)), i);
        if (result == SUCCESS) continue;
        if (result == UNCHANGED) {
          Append("null");
        } else {
          return result;
        }
      }
      break;
    }
    // TODO(yangguo):  The FAST_HOLEY_* cases could be handled in a faster way.
    // They resemble the non-holey cases except that a prototype chain lookup
    // is necessary for holes.
    default: {
      Result result = SerializeJSArraySlow(object, length);
      if (result != SUCCESS) return result;
      break;
    }
  }
  Append(']');
  StackPop();
  current_part_ = handle_scope.CloseAndEscape(current_part_);
  return SUCCESS;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeJSArraySlow(
    Handle<JSArray> object, int length) {
  for (int i = 0; i < length; i++) {
    if (i > 0) Append(',');
    Handle<Object> element = Object::GetElement(object, i);
    if (element->IsUndefined()) {
      Append("null");
    } else {
      Result result = SerializeElement(element, i);
      if (result == SUCCESS) continue;
      if (result == UNCHANGED) {
        Append("null");
      } else {
        return result;
      }
    }
  }
  return SUCCESS;
}


BasicJsonStringifier::Result BasicJsonStringifier::SerializeJSObject(
    Handle<JSObject> object) {
  HandleScope handle_scope(isolate_);
  Result stack_push = StackPush(object);
  if (stack_push != SUCCESS) return stack_push;
  if (object->IsJSGlobalProxy()) {
    object = Handle<JSObject>(JSObject::cast(object->GetPrototype()));
    ASSERT(object->IsGlobalObject());
  }
  bool has_exception = false;
  Handle<FixedArray> contents =
      GetKeysInFixedArrayFor(object, LOCAL_ONLY, &has_exception);
  if (has_exception) return EXCEPTION;
  Append('{');
  bool comma = false;
  for (int i = 0; i < contents->length(); i++) {
    Object* key = contents->get(i);
    Handle<String> key_handle;
    Handle<Object> property;
    if (key->IsString()) {
      key_handle = Handle<String>(String::cast(key));
      property = GetProperty(object, key_handle);
    } else {
      ASSERT(key->IsNumber());
      key_handle = isolate_->factory()->NumberToString(Handle<Object>(key));
      uint32_t index;
      if (key->IsSmi()) {
        property = Object::GetElement(object, Smi::cast(key)->value());
      } else if (key_handle->AsArrayIndex(&index)) {
        property = Object::GetElement(object, index);
      } else {
        property = GetProperty(object, key_handle);
      }
    }
    if (property.is_null()) return EXCEPTION;
    Result result = SerializeProperty(property, comma, key_handle);
    if (!comma && result == SUCCESS) comma = true;
    if (result >= EXCEPTION) return result;
  }
  Append('}');
  StackPop();
  current_part_ = handle_scope.CloseAndEscape(current_part_);
  return SUCCESS;
}


void BasicJsonStringifier::ShrinkCurrentPart() {
  ASSERT(current_index_ < part_length_);
  if (current_index_ == 0) {
    current_part_ = isolate_->factory()->empty_string();
    return;
  }

  int string_size, allocated_string_size;
  if (is_ascii_) {
    allocated_string_size = SeqAsciiString::SizeFor(part_length_);
    string_size = SeqAsciiString::SizeFor(current_index_);
  } else {
    allocated_string_size = SeqTwoByteString::SizeFor(part_length_);
    string_size = SeqTwoByteString::SizeFor(current_index_);
  }

  int delta = allocated_string_size - string_size;
  current_part_->set_length(current_index_);

  // String sizes are pointer size aligned, so that we can use filler objects
  // that are a multiple of pointer size.
  Address end_of_string = current_part_->address() + string_size;
  isolate_->heap()->CreateFillerObjectAt(end_of_string, delta);
  if (Marking::IsBlack(Marking::MarkBitFrom(*current_part_))) {
    MemoryChunk::IncrementLiveBytesFromMutator(
        current_part_->address(), -delta);
  }
}


template <bool is_ascii>
void BasicJsonStringifier::Extend() {
  set_accumulator(
      isolate_->factory()->NewConsString(accumulator(), current_part_));
  if (part_length_ <= kMaxPartLength / kPartLengthGrowthFactor) {
    part_length_ *= kPartLengthGrowthFactor;
  }
  if (is_ascii) {
    current_part_ =
        isolate_->factory()->NewRawAsciiString(part_length_);
  } else {
    current_part_ =
        isolate_->factory()->NewRawTwoByteString(part_length_);
  }
  current_index_ = 0;
}


void BasicJsonStringifier::ChangeEncoding() {
  ShrinkCurrentPart();
  set_accumulator(
      isolate_->factory()->NewConsString(accumulator(), current_part_));
  current_part_ =
      isolate_->factory()->NewRawTwoByteString(part_length_);
  current_index_ = 0;
  is_ascii_ = false;
}


template <typename SrcChar, typename DestChar>
void BasicJsonStringifier::SerializeStringUnchecked_(const SrcChar* src,
                                                     DestChar* dest,
                                                     int length) {
  dest += current_index_;
  DestChar* dest_start = dest;

  // Assert that uc16 character is not truncated down to 8 bit.
  // The <uc16, char> version of this method must not be called.
  ASSERT(sizeof(*dest) >= sizeof(*src));

  *(dest++) = '"';
  for (int i = 0; i < length; i++) {
    SrcChar c = src[i];
    if (DoNotEscape(c)) {
      *(dest++) = static_cast<DestChar>(c);
    } else {
      const char* chars = &JsonEscapeTable[c * kJsonEscapeTableEntrySize];
      while (*chars != '\0') *(dest++) = *(chars++);
    }
  }

  *(dest++) = '"';
  current_index_ += static_cast<int>(dest - dest_start);
}


template <bool is_ascii, typename Char>
void BasicJsonStringifier::SerializeString_(Vector<const Char> vector,
                                            Handle<String> string) {
  int length = vector.length();
  // We make a rough estimate to find out if the current string can be
  // serialized without allocating a new string part. The worst case length of
  // an escaped character is 6.  Shifting left by 3 is a more pessimistic
  // estimate than multiplying by 6, but faster to calculate.
  static const int kEnclosingQuotesLength = 2;
  if (current_index_ + (length << 3) + kEnclosingQuotesLength < part_length_) {
    if (is_ascii) {
      SerializeStringUnchecked_(
          vector.start(),
          SeqAsciiString::cast(*current_part_)->GetChars(),
          length);
    } else {
      SerializeStringUnchecked_(
          vector.start(),
          SeqTwoByteString::cast(*current_part_)->GetChars(),
          length);
    }
  } else {
    Append_<is_ascii, char>('"');
    String* string_location = *string;
    for (int i = 0; i < length; i++) {
      Char c = vector[i];
      if (DoNotEscape(c)) {
        Append_<is_ascii, Char>(c);
      } else {
        Append_<is_ascii, char>(
            &JsonEscapeTable[c * kJsonEscapeTableEntrySize]);
      }
      // If GC moved the string, we need to refresh the vector.
      if (*string != string_location) {
        vector = GetCharVector<Char>(string);
        string_location = *string;
      }
    }
    Append_<is_ascii, char>('"');
  }
}


template <>
bool BasicJsonStringifier::DoNotEscape(char c) {
  return c >= '#' && c <= '~' && c != '\\';
}


template <>
bool BasicJsonStringifier::DoNotEscape(uc16 c) {
  return (c >= 0x80) || (c >= '#' && c <= '~' && c != '\\');
}


template <>
Vector<const char> BasicJsonStringifier::GetCharVector(Handle<String> string) {
  String::FlatContent flat = string->GetFlatContent();
  ASSERT(flat.IsAscii());
  return flat.ToAsciiVector();
}


template <>
Vector<const uc16> BasicJsonStringifier::GetCharVector(Handle<String> string) {
  String::FlatContent flat = string->GetFlatContent();
  ASSERT(flat.IsTwoByte());
  return flat.ToUC16Vector();
}


void BasicJsonStringifier::SerializeString(Handle<String> object) {
  FlattenString(object);
  String::FlatContent flat = object->GetFlatContent();
  if (is_ascii_) {
    if (flat.IsAscii()) {
      SerializeString_<true, char>(flat.ToAsciiVector(), object);
    } else {
      ChangeEncoding();
      SerializeString(object);
    }
  } else {
    if (flat.IsAscii()) {
      SerializeString_<false, char>(flat.ToAsciiVector(), object);
    } else {
      SerializeString_<false, uc16>(flat.ToUC16Vector(), object);
    }
  }
}

} }  // namespace v8::internal

#endif  // V8_JSON_STRINGIFIER_H_