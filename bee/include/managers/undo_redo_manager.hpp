#pragma once
#include "common.hpp"
#include "ecs/components.hpp"

namespace bee
{
class Command
{
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void OnImGuiRender() = 0;
};

class UndoRedoManager
{
public:
    static void Execute(Scope<Command> command);

    static void Undo();
    static void Redo();
    static void OnImGuiRender();

    static std::unordered_map<entt::entity, entt::entity> EntityMap;

private:
    static std::stack<Ref<Command>> m_undoStack;
    static std::stack<Ref<Command>> m_redoStack;
};

// ====== Command Combination ======
class CommandCombination : public Command
{
public:
    CommandCombination(std::vector<Scope<Command>> commands);

    void Execute() override;
    void Undo() override;
    void OnImGuiRender() override;

private:
    std::vector<Scope<Command>> m_commands;
};

// ====== Model Command ======
class ModelMatrixCommand : public Command
{
public:
    ModelMatrixCommand(entt::entity target, const glm::mat4& newModelMatrix);
    ModelMatrixCommand(entt::entity target, const glm::mat4& previousModelMatrix, const glm::mat4& newModelMatrix);

    void Execute() override;
    void Undo() override;
    void OnImGuiRender() override;

private:
    entt::entity m_target;
    glm::mat4 m_previousModelMatrix;
    glm::mat4 m_newModelMatrix;
};

// ====== Create entity command ======
class CreateEntityCommand : public Command
{
public:
    CreateEntityCommand(entt::entity entity);
    void Execute() override;
    void Undo() override;
    void OnImGuiRender() override;

private:
    std::string m_entityData;
    entt::entity m_entity;
};

// ====== Delete entity command ======
class DeleteEntityCommand : public Command
{
public:
    // <summary>
    // Make sure you call this before deleting the entity
    // </summary>
    DeleteEntityCommand(entt::entity entity);
    void Execute() override;
    void Undo() override;
    void OnImGuiRender() override;

private:
    std::string m_entityData;
    entt::entity m_entity;
};

// ====== any type command ======
template <typename T>
class TypeCommand : public Command
{
public:
    TypeCommand(T& target, T newValue) : m_target(target), m_newValue(newValue) { m_previousValue = target; }
    TypeCommand(T& target, T previousValue, T newValue) : m_target(target), m_previousValue(previousValue), m_newValue(newValue) {}

    void Execute() override { m_target = m_newValue; }

    void Undo() override { m_target = m_previousValue; }

    void OnImGuiRender() override
    {
        ImGui::Text("Type Command");
    }

private:
    T& m_target;
    T m_previousValue;
    T m_newValue;
};

}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/