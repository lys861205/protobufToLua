#include <lua.hpp>

#ifdef __cplusplus
extern "C" {
#endif 

#define NARR 20
#define NREC 40

int Proto2Table(const google::protobuf::Message* message, 
                  const google::protobuf::Descriptor* descriptor, 
                  const google::protobuf::Reflection* reflection,
                  lua_State* l);
int repeated2Table(const google::protobuf::Message* message, 
                    const google::protobuf::Descriptor* descriptor,
                    const google::protobuf::Reflection* reflection,
                    const google::protobuf::FieldDescriptor* field,
                    lua_State* l);
int singular2Table(const google::protobuf::Message* message, 
                    const google::protobuf::Descriptor* descriptor,
                    const google::protobuf::Reflection* reflection,
                    const google::protobuf::FieldDescriptor* field,
                    lua_State* l);

int Proto2Table(const google::protobuf::Message* message, 
                  const google::protobuf::Descriptor* descriptor, 
                  const google::protobuf::Reflection* reflection,
                  lua_State* l)
{
  lua_createtable(l, NARR, NREC);
  //lua_newtable(l);
  for (int i=0; i < descriptor->field_count(); i++)
  {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i); 
    if (field->is_repeated())
    {
      int num = repeated2Table(message, descriptor, reflection, field, l);
      if (-1 == num)
      {
        continue;
      }
      lua_rawset(l, -3);
      std::string field_size = field->name();
      field_size.append("_count");
      lua_pushlstring(l, field_size.c_str(), field_size.size());
      lua_pushinteger(l, num);
      lua_rawset(l, -3);
    } 
    else 
    {
      if ( -1 == singular2Table(message, descriptor, reflection, field, l) )
      {
        continue;
      }
      lua_rawset(l, -3);
    }
  }
  return 0;
}


int repeated2Table(const google::protobuf::Message* message, 
                    const google::protobuf::Descriptor* descriptor,
                    const google::protobuf::Reflection* reflection,
                    const google::protobuf::FieldDescriptor* field,
                    lua_State* l)
{
  int field_num = reflection->FieldSize(*message, field);   
  if ( field_num <= 0 ){
    return -1;
  }
  // key
  std::string field_name = field->name();
  lua_pushlstring(l, field_name.c_str(), field_name.size());

  //lua_newtable(l);
  lua_createtable(l, NARR, NREC);
  for (int j=0; j < field_num; ++j)
  {
    switch(field->cpp_type())
    {
      #define CASE_GET_REPEATED_FIELD(cpptype, method, type, luamethod) \
        case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype: \
          { \
            type value = reflection->GetRepeated##method(*message, field, j);\
            lua_push##luamethod(l, value);\
            break;\
          }

      CASE_GET_REPEATED_FIELD(INT32, Int32, int32_t, integer);
      CASE_GET_REPEATED_FIELD(UINT32, UInt32, uint32_t, integer);
      CASE_GET_REPEATED_FIELD(INT64, Int64, int64_t, integer);
      CASE_GET_REPEATED_FIELD(UINT64, UInt64, uint64_t, integer);
      CASE_GET_REPEATED_FIELD(FLOAT, Float, float, number);
      CASE_GET_REPEATED_FIELD(DOUBLE, Double, double, number);
      CASE_GET_REPEATED_FIELD(BOOL, Bool, bool, boolean);

      case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      {
        int32_t value = reflection->GetRepeatedEnum(*message, field, j)->number(); 
        lua_pushinteger(l, value);
        break;
      }
      case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
      {
        std::string value = reflection->GetRepeatedString(*message, field, j);
        lua_pushlstring(l, value.c_str(), value.size());
        break;
      }
      case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      {
        const google::protobuf::Message& submsg = reflection->GetRepeatedMessage(*message, field, j);
        const google::protobuf::Descriptor* subdesc = submsg.GetDescriptor();
        const google::protobuf::Reflection* subrefl = submsg.GetReflection();
        Proto2Table(&submsg, subdesc, subrefl, l);
        break;
      }
    }
    lua_rawseti(l, -2, j+1);
  }
  return field_num;
}

int singular2Table(const google::protobuf::Message* message, 
                    const google::protobuf::Descriptor* descriptor,
                    const google::protobuf::Reflection* reflection,
                    const google::protobuf::FieldDescriptor* field,
                    lua_State* l)
{
  if(!reflection->HasField(*message, field)){
    return -1;
  }
    // key
  std::string field_name = field->name();
  lua_pushlstring(l, field_name.c_str(), field_name.size());
  switch(field->cpp_type())
  {
    #define CASE_GET_FIELD_VALUE(cpptype, c_method_suffix, type, lua_method_suffix) \
      case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:\
      {\
        type value = reflection->Get##c_method_suffix(*message, field);\
        lua_push##lua_method_suffix(l, value);\
        break;\
      }
    // value
    CASE_GET_FIELD_VALUE(INT32, Int32, int32_t, integer)
    CASE_GET_FIELD_VALUE(UINT32, UInt32, uint32_t, integer)
    CASE_GET_FIELD_VALUE(FLOAT, Float, float, number)
    CASE_GET_FIELD_VALUE(DOUBLE, Double, double, number)
    CASE_GET_FIELD_VALUE(INT64, Int64, int64_t, integer)
    CASE_GET_FIELD_VALUE(UINT64, UInt64, uint64_t, integer)
    CASE_GET_FIELD_VALUE(BOOL, Bool, bool, boolean)

    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
    {
      int32_t value = reflection->GetEnum(*message, field)->number(); 
      lua_pushinteger(l, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    {
      std::string value = reflection->GetString(*message, field);
      lua_pushlstring(l, value.c_str(), value.size());
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
    {
      const google::protobuf::Message& submsg = reflection->GetMessage(*message, field);
      const google::protobuf::Descriptor* subdesc = submsg.GetDescriptor();
      const google::protobuf::Reflection* subrefl = submsg.GetReflection();
      Proto2Table(&submsg, subdesc, subrefl, l);
      break;
    }
  }
  return 0;
}

int ConvertToTable(const google::protobuf::Message* message, lua_State* l)
{
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  const google::protobuf::Reflection* reflection = message->GetReflection();
  return Proto2Table(message, descriptor, reflection, l);
}


#ifdef __cplusplus
}
#endif 
