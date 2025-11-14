#pragma once

#include <vector>
#include <string>
#include <iostream>

namespace ArgumentParser {
    class ArgParser {
        class Argument {
        public:
            Argument(): name("Unnamed"), longName("No Long Name"), description("No Description"){}
            virtual ~Argument() = default;
            std::string name;
            std::string longName;
            std::string description;
            std::string defaultValue;

            bool isDefault = false;
            bool isStored = false;
            bool isMultiValue = false;
            bool isPositional = false;

            uint32_t minCount = 0;

            virtual bool isValid() const = 0;
            virtual void store() = 0;
            virtual bool wordFind(std::string word) = 0;
            virtual bool wordConstFind(std::string word) = 0;
            virtual Argument& MultiValue() = 0;
            virtual Argument& MultiValue(const int& count) = 0;
            virtual Argument& Positional() = 0;
        };

        class StringArgument : public Argument{
        public:
            StringArgument() = default;
            std::string* storedValue = nullptr;
            std::vector<std::string> values;
            std::vector<std::string>* storedValues = nullptr;

            bool wordFind(std::string word) override;

            bool wordConstFind(std::string word) override;

            bool isValid() const override;

            void store() override;

            StringArgument& Default(const std::string& def);

            StringArgument& StoreValue(std::string& value);

            StringArgument& StoreValues(std::vector<std::string>& values);

            StringArgument& MultiValue() override;

            StringArgument& MultiValue(const int& count) override;

            StringArgument& Positional() override;
        };

        class IntArgument : public Argument{
            public:
                IntArgument() = default;
                int defaultValue = 0;
                int* storedValue = nullptr;

                std::vector<int> values;
                std::vector<int>* storedValues = nullptr;

                virtual bool wordFind(std::string word) override;

                virtual bool wordConstFind(std::string word) override;

                void store() override;

                bool isValid() const override;

                IntArgument& Default(const int& def);

                IntArgument& StoreValue(int& value);

                IntArgument& StoreValues(std::vector<int>& values);

                IntArgument& MultiValue() override;

                IntArgument& MultiValue(const int& count) override;

                IntArgument& Positional() override;
        };

        class FlagArgument : public Argument{
            public:
                FlagArgument() = default;
                bool defaultValue;
                bool* storedValue = nullptr;

                bool value;

                virtual bool wordFind(std::string word) override;

                virtual bool wordConstFind(std::string word) override;

                void setValue(bool val);

                void store() override;

                bool isValid() const override;

                FlagArgument& Default(const bool& def);

                FlagArgument& StoreValue(bool& value);

                FlagArgument& MultiValue() override;

                FlagArgument& MultiValue(const int& count) override;

                FlagArgument& Positional() override;
        };
        public:
            ArgParser(const std::string& name) : progName(name){}
            ~ArgParser(){
                for (auto arg : args){
                    delete arg;
                }
            }

            std::string GetName(const ArgParser& parser) const{
                return parser.progName;
            }

            ArgParser(const ArgParser& other) = delete;

            ArgParser& operator=(const ArgParser& other) = delete;

            bool Parse(std::vector<std::string> args);

            bool Parse(int argc, char** argv);

            bool isCorrect();

            void setStored();

            //string
            StringArgument& AddStringArgument(const std::string& longName, const std::string& desc = "Some desc");
            StringArgument& AddStringArgument(char name,
                                              const std::string& longName,
                                              const std::string& desc = "Some desc");
            std::string GetStringValue(const std::string& arg, int index = 0) const;
            //int
            IntArgument& AddIntArgument(const std::string& longName, const std::string& desc = "Some desc");
            IntArgument& AddIntArgument(char name,
                                        const std::string& longName,
                                        const std::string& desc = "Some desc");
            int GetIntValue(const std::string& arg, int index = 0) const;
            //bool
            FlagArgument& AddFlag(const std::string& longName, const std::string& desc = "Some desc");
            FlagArgument& AddFlag(char name,
                                  const std::string& longName,
                                  const std::string& desc = "Some desc");
            bool GetFlag(const std::string& arg) const;
            //help
            void AddHelp(const std::string& longName, const std::string& desc = "Some desc");
            void AddHelp(char name,
                         const std::string& longName,
                         const std::string& desc = "Some desc");
            std::string HelpDescription() const;
            bool Help() const;

            bool isNumber(const std::string& num);

        private:
            std::string progName;

            std::vector<Argument*> args;

            std::string helpName;
            std::string longHelpName;
            std::string helpDescription;

            bool isHelp = false;
            bool isHelpUsed = false;
    };
}