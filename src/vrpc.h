#ifndef VRPC_H
#define VRPC_H

#ifdef WITH_STL
#include <ArduinoSTL.h>
#endif
#include <ArduinoJson.h>
#include <ArduinoUniqueID.h>
#include <MQTT.h>
#include <map>
#include <vector>
#include "notstd.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace vrpc {

const String compile_date = __DATE__ " " __TIME__;

typedef DynamicJsonDocument Json;

// Singleton helper
template <typename T>
inline T& init() {
  static T t;
  return t;
}

namespace details {

// The code below was formulated as an answer to StackOverflow and can be read
// here:
// http://stackoverflow.com/questions/10766112/c11-i-can-go-from-multiple-args-to-tuple-but-can-i-go-from-tuple-to-multiple

template <typename R,
          typename F,
          typename Tuple,
          bool Done,
          int Total,
          int... N>
struct call_impl {
  static R call(F f, Tuple&& t) {
    return call_impl<R, F, Tuple, Total == 1 + sizeof...(N), Total, N...,
                     sizeof...(N)>::call(f, notstd::forward<Tuple>(t));
  }
};

template <typename R, typename F, typename Tuple, int Total, int... N>
struct call_impl<R, F, Tuple, true, Total, N...> {
  static R call(F f, Tuple t) {
    // FIXME: This should really perfectly forward here, but does not compile
    // yet return f(notstd::get<N>(notstd::forward<Tuple>(t))...);
    return f(notstd::get<N>(t)...);
  }
};
}  // namespace details

/**
 * Call a function f with arguments unpacked from a notstd::tuple
 * @param f The function to be called
 * @param t A tuple containing the function arguments
 */
template <typename R, typename F, typename Tuple>
R call(F f, Tuple&& t) {
  typedef typename notstd::decay<Tuple>::type ttype;
  return details::call_impl<
      R, F, Tuple, 0 == notstd::tuple_size<ttype>::value,
      notstd::tuple_size<ttype>::value>::call(f, notstd::forward<Tuple>(t));
}

// unpack

namespace details {

template <size_t N, typename Ret, typename... Args>
struct unpack_impl;

template <size_t N, typename Ret, typename Arg, typename... Args>
struct unpack_impl<N, Ret, Arg, Args...> {
  template <typename A = Arg, typename R = Ret>
  static void unpack(const Json& j, R& t) {
    // length 4 for better assembly alignment
    constexpr const char key[] = {'_', N + 49, '\0', '\0'};
    typedef typename notstd::decay<A>::type dA;
    notstd::get<N>(t) = j["data"][key].as<dA>();
    unpack_impl<N + 1, Ret, Args...>::unpack(j, t);
  }
};

template <size_t N, typename Ret, typename Arg>
struct unpack_impl<N, Ret, Arg> {
  template <typename A = Arg, typename R = Ret>
  static void unpack(const Json& j, R& t) {
    constexpr const char key[] = {'_', N + 49, '\0', '\0'};
    typedef typename notstd::decay<A>::type dA;
    notstd::get<N>(t) = j["data"][key].as<dA>();
  }
};

template <size_t N, typename Ret>
struct unpack_impl<N, Ret> {
  template <typename R = Ret>
  static void unpack(const Json&, R&) {
    // Do nothing
  }
};

}  // namespace details

template <typename... Args>
notstd::tuple<Args...> unpack(const Json& j) {
  typedef typename notstd::tuple<Args...> Ret;
  Ret t;
  details::unpack_impl<0, Ret, Args...>::unpack(j, t);
  return t;
}

class AbstractFunction {
 public:
  AbstractFunction() = default;

  virtual ~AbstractFunction() = default;

  void call_function(Json& j) { this->do_call_function(j); }

 protected:
  virtual void do_call_function(Json&) = 0;
};

template <typename R, typename... Args>
class GlobalFunction : public AbstractFunction {
  notstd::function<R(Args...)> _f;

 public:
  template <typename F>
  GlobalFunction(F&& f) : _f(f) {}
  virtual ~GlobalFunction() = default;

  virtual void do_call_function(Json& j) {
    j["data"]["r"] = call<R>(_f, unpack<Args...>(j));
  }
};

template <typename... Args>
class GlobalFunction<void, Args...> : public AbstractFunction {
  notstd::function<void(Args...)> _f;

 public:
  template <typename F>
  GlobalFunction(F&& f) : _f(f) {}
  virtual ~GlobalFunction() = default;

  virtual void do_call_function(Json& j) {
    call<void>(_f, unpack<Args...>(j));
    j["data"]["r"] = nullptr;
  }
};

class Registry {
  friend Registry& init<Registry>();
  typedef std::map<String, AbstractFunction*> StringFunctionMap;
  typedef std::map<String, StringFunctionMap> FunctionRegistry;

  FunctionRegistry _function_registry;

 public:
  template <typename Func, Func f, typename R, typename... Args>
  static void register_global_function(const String& function_name) {
    AbstractFunction* func = new GlobalFunction<R, Args...>(f);
    init<Registry>()._function_registry["__global__"][function_name] = func;
  }

  static String call(const String& jsonString) {
    Json json(256);
    DeserializationError err = deserializeJson(json, jsonString);
    if (err) {
      Serial.print(F("ERROR [VRPC] JSON parsing failed because: "));
      Serial.println(err.c_str());
      return String("{\"data\": {\"e\": \"JSON parsing failed because: ") +
             String(err.c_str()) + String("\"}}");
    }
    Registry::call(json);
    String res;
    serializeJson(json, res);
    return res;
  }

  static void call(Json& json) {
    String context = json["context"].as<String>();
    String function_name = json["method"].as<String>();
    // TODO implement support for overloading
    // JsonVariant args = json["data"];
    // function_name += vrpc::get_signature(args);
    auto it_t = init<Registry>()._function_registry.find(context);
    if (it_t != init<Registry>()._function_registry.end()) {
      auto it_f = it_t->second.find(function_name);
      if (it_f != it_t->second.end()) {
        it_f->second->call_function(json);
      } else {
        Serial.print("ERROR [VRPC] Could not find function: ");
        Serial.println(function_name);
        json["data"]["e"] = "Could not find function: " + function_name;
      }
    } else {
      Serial.print("ERROR [VRPC] Could not find context: ");
      Serial.println(context);
      json["data"]["e"] = "Could not find context: " + context;
    }
  }

  static std::vector<String> get_classes() {
    std::vector<String> classes;
    for (const auto& kv : init<Registry>()._function_registry) {
      classes.push_back(kv.first);
    }
    return classes;
  }

  static std::vector<String> get_static_functions(const String& class_name) {
    std::vector<String> functions;
    const auto& it = init<Registry>()._function_registry.find(class_name);
    if (it != init<Registry>()._function_registry.end()) {
      for (const auto& kv : it->second) {
        functions.push_back(kv.first);
      }
    }
    return functions;
  }
};

template <typename Func, Func f, typename R, typename... Args>
struct GlobalFunctionRegistrar {
  GlobalFunctionRegistrar(const String& function_name) {
    Registry::register_global_function<Func, f, R, Args...>(function_name);
  }
};

template <typename Func, Func f, typename R, typename... Args>
struct RegisterGlobalFunction {
  static const GlobalFunctionRegistrar<Func, f, R, Args...> registerAs;
};

}  // namespace vrpc

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @brief Agent that transforms existing code to be operated from a remote
 * location.
 *
 */
class VrpcAgent {
  MQTTClient _client;
  String _domain_agent;
  String _token;
  String _username;
  String _broker;

 public:
  /**
   * @brief Constructs an agent
   *
   * @param maxBytesPerMessage [optional, default: `1024`] Specifies the maximum
   * size a single MQTT message may have
   */
  VrpcAgent(int maxBytesPerMessage = 1024) : _client(maxBytesPerMessage) {}

  /**
   * @brief Initializes the object using a client class for network transport
   *
   * @tparam T Type of the client class
   * @param wifiClient A client class following the interface as described
   * [here](https://www.arduino.cc/en/Reference/ClientConstructor)
   * @param domain [optional, default: `"public.vrpc"`] The domain under which
   * the agent-provided code is reachable
   * @param token [optional] Access token as generated using the [VRPC
   * App](https://app.vrpc.io), or password if own broker is used
   * @param broker [optional, default: `"vrpc.io"`] Address of the MQTT broker
   * @param username [optional] MQTT username (not needed when using the vrpc.io
   * broker)
   */
  template <typename T>
  void begin(T& wifiClient,
             const String& domain = "public.vrpc",
             const String& token = "",
             const String& broker = "vrpc.io",
             const String& username = "") {
    _domain_agent = domain + "/" + VrpcAgent::get_unique_id();
    _token = token;
    _username = username;
    _broker = broker;
    _client.begin(broker.c_str(), 1883, wifiClient);
    _client.onMessageAdvanced(on_message);
    String willTopic(_domain_agent + "/__agentInfo__");
    String payload = this->create_agent_info_payload(false);
    _client.setWill(willTopic.c_str(), payload.c_str(), true, 1);
  }

  /**
   * @brief Reports the current connectivity status
   *
   * @return true when connected, false otherwise
   */
  bool connected() { return _client.connected(); }

  /**
   * @brief Connect the agent to the broker.
   *
   * The function will try to connect forever. Inspect the serial monitor
   * to see the connectivity progress.
   */
  void connect() {
    Serial.print("\nConnecting to MQTT broker...");
    Serial.print("\ndomain/agent: ");
    Serial.print(_domain_agent);
    Serial.print("\nbroker: ");
    Serial.print(_broker);
    String clientId = "vrpca" + VrpcAgent::get_unique_id();
    if (_token == "" && _username == "") {
      while (!(_client.connect(clientId.c_str())))
        delay(1000);
    } else if (_username != "") {
      while (!_client.connect(clientId.c_str()), _username.c_str(),
             _token.c_str())
        delay(1000);
    } else {
      while (!_client.connect(clientId.c_str(), "__token__", _token.c_str()))
        delay(1000);
    }
    Serial.println("\n[OK]\n");
    publish_agent_info();
    const auto& classes = vrpc::Registry::get_classes();
    for (const auto& class_name : classes) {
      publish_class_info(class_name);
      const auto& functions = vrpc::Registry::get_static_functions(class_name);
      for (const auto& func : functions) {
        _client.subscribe(_domain_agent + "/" + class_name + "/__static__/" +
                          func);
      }
    }
  }

  /**
   * @brief This function will send and receive VRPC packets
   *
   * **IMPORTANT**: This function should be called in every `loop`
   */
  void loop() {
    _client.loop();
    auto lastError = _client.lastError();
    if (lastError != LWMQTT_SUCCESS) {
      Serial.print("ERROR [VRPC] Mqtt transport failed because: ");
      Serial.println(lastError);
      delay(2000);
    }
    delay(10);
  }

 private:
  static void on_message(MQTTClient* client,
                         char* topic,
                         char* payload,
                         int size) {
    String topicString(topic);
    std::vector<String> tokens = VrpcAgent::tokenize(topicString, '/');
    if (tokens.size() != 5) {
      Serial.println("ERROR [VRPC] Received invalid message");
      return;
    }
    String class_name(tokens[2]);
    String instance(tokens[3]);
    String method(tokens[4]);
    vrpc::Json j(256);
    deserializeJson(j, payload, size);
    j["context"] = instance == "__static__" ? class_name : instance;
    j["method"] = method;
    Serial.print("Going to call: ");
    Serial.println(method);
    vrpc::Registry::call(j);
    String res;
    serializeJson(j, res);
    client->publish(j["sender"].as<String>(), res);
  }

  static String get_unique_id() {
    UniqueIDdump(Serial);
    String id = "ar";
    for (size_t i = 0; i < 8; i++) {
      if (UniqueID8[i] < 0x10) {
        id += "0";
      }
      id += String(UniqueID8[i], HEX);
    }
    return id;
  }

  void publish_agent_info() {
    String topic(_domain_agent + "/__agentInfo__");
    String json = this->create_agent_info_payload(true);
    Serial.print("Sending AgentInfo...");
    Serial.println(json);
    _client.publish(topic, json, true, 1);
    Serial.println("[OK]");
  }

  String create_agent_info_payload(bool isOnline) {
    return isOnline ? "{\"status\":\"online\",\"hostname\":\"arduino-board\"}"
                    : "{\"status\":\"offline\",\"hostname\":\"arduino-board\"}";
  }

  void publish_class_info(const String& class_name) {
    String json("{\"className\":\"");
    json += class_name +
            "\",\"instances\":[],\"memberFunctions\":[],\"staticFunctions\":[";
    const auto& functions = vrpc::Registry::get_static_functions(class_name);
    for (const auto& func : functions)
      json += "\"" + func + "\",";
    json[json.length() - 1] = ']';
    json += "}";
    const String topic(_domain_agent + "/" + class_name + "/__classInfo__");
    Serial.print("Sending ClassInfo...");
    Serial.println(json);
    _client.publish(topic, json, true, 1);
    Serial.println("[OK]");
  }

  static std::vector<String> tokenize(const String& input, char delim) {
    std::vector<String> output;
    size_t pos = 0;
    for (size_t i = 0; i < input.length(); ++i) {
      if (input[i] == delim) {
        output.push_back(input.substring(pos, i));
        pos = i + 1;
      }
    }
    output.push_back(input.substring(pos));
    return output;
  }
};

/*----------------------------- Macro utility --------------------------------*/

#define CAT(A, B) A##B
#define SELECT(NAME, NUM) CAT(_##NAME##_, NUM)
#define GET_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, COUNT, ...) COUNT
#define VA_SIZE(...) GET_COUNT(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define VA_SELECT(NAME, ...) SELECT(NAME, VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

#define VRPC_GLOBAL_FUNCTION(...) VA_SELECT(VRPC_GLOBAL_FUNCTION, __VA_ARGS__)

/*---------------------------- Zero arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_2(Ret, Function)                                 \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)()>(Function)), &Function, Ret>              \
      vrpc::RegisterGlobalFunction<decltype(static_cast<Ret (*)()>(Function)), \
                                   &Function, Ret>::registerAs(#Function);

/*----------------------------- One argument ---------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_3(Ret, Function, A1)                      \
  template <>                                                           \
  const vrpc::GlobalFunctionRegistrar<                                  \
      decltype(static_cast<Ret (*)(A1)>(Function)), &Function, Ret, A1> \
      vrpc::RegisterGlobalFunction<decltype(static_cast<Ret (*)(A1)>(   \
                                       Function)),                      \
                                   &Function, Ret, A1>::registerAs(#Function);

/*----------------------------- Two arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_4(Ret, Function, A1, A2)                       \
  template <>                                                                \
  const vrpc::GlobalFunctionRegistrar<decltype(static_cast<Ret (*)(A1, A2)>( \
                                          Function)),                        \
                                      &Function, Ret, A1, A2>                \
      vrpc::RegisterGlobalFunction<                                          \
          decltype(static_cast<Ret (*)(A1, A2)>(Function)), &Function, Ret,  \
          A1, A2>::registerAs(#Function);

/*--------------------------- Three arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_5(Ret, Function, A1, A2, A3)                  \
  template <>                                                               \
  const vrpc::GlobalFunctionRegistrar<                                      \
      decltype(static_cast<Ret (*)(A1, A2, A3)>(Function)), &Function, Ret, \
      A1, A2, A3>                                                           \
      vrpc::RegisterGlobalFunction<                                         \
          decltype(static_cast<Ret (*)(A1, A2, A3)>(Function)), &Function,  \
          Ret, A1, A2, A3>::registerAs(#Function);

/*---------------------------- Four arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_6(Ret, Function, A1, A2, A3, A4)                 \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Function)), &Function,     \
      Ret, A1, A2, A3, A4>                                                     \
      vrpc::RegisterGlobalFunction<                                            \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Function)), &Function, \
          Ret, A1, A2, A3, A4>::registerAs(#Function);

/*---------------------------- Five arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_7(Ret, Function, A1, A2, A3, A4, A5)             \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Function)), &Function, \
      Ret, A1, A2, A3, A4, A5>                                                 \
      vrpc::RegisterGlobalFunction<                                            \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Function)),        \
          &Function, Ret, A1, A2, A3, A4, A5>::registerAs(#Function);

/*----------------------------- Six arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_8(Ret, Function, A1, A2, A3, A4, A5, A6)      \
  template <>                                                               \
  const vrpc::GlobalFunctionRegistrar<                                      \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Function)),     \
      &Function, Ret, A1, A2, A3, A4, A5, A6>                               \
      vrpc::RegisterGlobalFunction<                                         \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Function)), \
          &Function, Ret, A1, A2, A3, A4, A5, A6>::registerAs(#Function);

#endif
