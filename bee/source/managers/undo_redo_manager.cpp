#include "core.hpp"
#include "managers/undo_redo_manager.hpp"

std::stack<Ref<bee::Command>> bee::UndoRedoManager::m_undoStack;
std::stack<Ref<bee::Command>> bee::UndoRedoManager::m_redoStack;
std::unordered_map<entt::entity, entt::entity> bee::UndoRedoManager::EntityMap;

void traverse_stack(std::stack<Ref<bee::Command>>& st, const std::function<void(const Ref<bee::Command>&, size_t index)>& f)
{
    std::vector<Ref<bee::Command>> tempVec;

    // Move elements from stack to vector for reversing the order
    while (!st.empty())
    {
        tempVec.push_back(std::move(st.top()));
        st.pop();
    }

    // Traverse the vector in reverse order to simulate bottom-to-top stack traversal
    for (size_t i = 0; i < tempVec.size(); ++i)
    {
        f(tempVec[i], i);
    }

    // Restore the stack from the vector (reversing it back to its original order)
    for (auto it = tempVec.rbegin(); it != tempVec.rend(); ++it)
    {
        st.push(std::move(*it));
    }
}

void bee::UndoRedoManager::Execute(Scope<Command> command)
{
    command->Execute();
    m_undoStack.push(std::move(command));
    // Clear the redo stack when a new command is executed
    while (!m_redoStack.empty()) m_redoStack.pop();
}

void bee::UndoRedoManager::Undo()
{
    if (!m_undoStack.empty())
    {
        Ref<Command> command = std::move(m_undoStack.top());
        m_undoStack.pop();
        command->Undo();
        m_redoStack.push(std::move(command));
    }
}

void bee::UndoRedoManager::Redo()
{
    if (!m_redoStack.empty())
    {
        Ref<Command> command = std::move(m_redoStack.top());
        m_redoStack.pop();
        command->Execute();
        m_undoStack.push(std::move(command));
    }
}

void bee::UndoRedoManager::OnImGuiRender()
{
    ImGui::Begin(ICON_FA_ARROWS_ROTATE TAB_FA "Undo/Redo Manager");
    if (ImGui::Button(ICON_FA_REPLY TAB_FA "Undo")) Undo();
    ImGui::SameLine();
    if (ImGui::Button("Redo" TAB_FA ICON_FA_SHARE)) Redo();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH TAB_FA "Clear"))
    {
        while (!m_undoStack.empty()) m_undoStack.pop();
        while (!m_redoStack.empty()) m_redoStack.pop();
    }

    // Set table flags for borders and outlines
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

    // Create a table with two columns
    if (ImGui::BeginTable("UndoRedoTable", 2, flags))
    {
        // Set the column widths
        ImGui::TableSetupColumn("Undo Stack", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Redo Stack", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // Start rendering rows
        size_t undoStackSize = m_undoStack.size();
        size_t redoStackSize = m_redoStack.size();
        size_t maxStackSize = std::max(undoStackSize, redoStackSize);

        for (size_t i = 0; i < maxStackSize; ++i)
        {
            ImGui::TableNextRow();  // Create a new row

            // Render Undo stack in the first column
            ImGui::TableNextColumn();
            if (i < undoStackSize)
            {
                traverse_stack(m_undoStack,
                               [&](const Ref<Command>& command, size_t index)
                               {
                                   if (index == i)
                                   {
                                       command->OnImGuiRender();
                                   }
                               });
            }

            // Render Redo stack in the second column
            ImGui::TableNextColumn();
            if (i < redoStackSize)
            {
                traverse_stack(m_redoStack,
                               [&](const Ref<Command>& command, size_t index)
                               {
                                   if (index == i)
                                   {
                                       command->OnImGuiRender();
                                   }
                               });
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

// ======= CommandCombination =======

bee::CommandCombination::CommandCombination(std::vector<Scope<Command>> commands) { this->m_commands = std::move(commands); }

void bee::CommandCombination::Execute()
{
    for (auto& command : m_commands) command->Execute();
}

void bee::CommandCombination::Undo()
{
    for (auto& command : m_commands) command->Undo();
}

void bee::CommandCombination::OnImGuiRender()
{
    if (m_commands.size() > 1) ImGui::Text("Command Combination:");
    if (m_commands.size() > 1) ImGui::Indent();
    for (auto& command : m_commands) command->OnImGuiRender();
    if (m_commands.size() > 1) ImGui::Unindent();
}

// ======= ModelCommand =======
bee::ModelMatrixCommand::ModelMatrixCommand(entt::entity target, const glm::mat4& newModelMatrix)
    : m_target(target), m_newModelMatrix(newModelMatrix)
{
    auto& registry = bee::Engine.Registry();
    if (!registry.valid(target)) return;
    m_previousModelMatrix = GetWorldModel(target, registry);
}

bee::ModelMatrixCommand::ModelMatrixCommand(entt::entity target,
                                            const glm::mat4& previousModelMatrix,
                                            const glm::mat4& newModelMatrix)
{
    this->m_target = target;
    this->m_previousModelMatrix = previousModelMatrix;
    this->m_newModelMatrix = newModelMatrix;

    // auto& registry = bee::Engine.Registry();
    // if (!registry.valid(target)) return;
    // SetWorldModel(target, newModelMatrix, registry);
}

void bee::ModelMatrixCommand::Execute()
{
    auto& registry = bee::Engine.Registry();
    if (!registry.valid(m_target))
    {
        // check the undo redo mapping to see if the entity was deleted
        if (bee::UndoRedoManager::EntityMap.find(m_target) != bee::UndoRedoManager::EntityMap.end())
        {
            m_target = bee::UndoRedoManager::EntityMap[m_target];
        }
        else
        {
            return;
        }
    }
    SetWorldModel(m_target, m_newModelMatrix, registry);
}

void bee::ModelMatrixCommand::Undo()
{
    auto& registry = bee::Engine.Registry();
    if (!registry.valid(m_target))
    {
        // check the undo redo mapping to see if the entity was deleted
        if (bee::UndoRedoManager::EntityMap.find(m_target) != bee::UndoRedoManager::EntityMap.end())
        {
            m_target = bee::UndoRedoManager::EntityMap[m_target];
        }
        else
        {
            return;
        }
    }
    SetWorldModel(m_target, m_previousModelMatrix, registry);
}

void bee::ModelMatrixCommand::OnImGuiRender() { ImGui::Text("Model Matrix Command"); }

// ======= CreateEntityCommand =======
bee::CreateEntityCommand::CreateEntityCommand(entt::entity entity) : m_entity(entity) {}

void bee::CreateEntityCommand::Execute()
{
    auto& registry = bee::Engine.Registry();
    // this check is here to prevent the command from being executed if the entity already exists
    if (registry.valid(m_entity)) return;
    std::unordered_map<entt::entity, entt::entity> entity_mapping;

    deserializeEntityString<cereal::JSONInputArchive>(m_entityData, registry, entity_mapping);
    bee::ecs::UpdateEntityMapping(entity_mapping, registry);

    m_entity = entity_mapping[m_entity];

    // add the entity mapping to the undo redo manager mapping
    for (auto& [oldEntity, newEntity] : entity_mapping)
    {
        bee::UndoRedoManager::EntityMap[oldEntity] = newEntity;
    }
}

void bee::CreateEntityCommand::Undo()
{
    auto& registry = bee::Engine.Registry();
    if (!registry.valid(m_entity))
    {
        // check the undo redo mapping to see if the entity was deleted
        if (bee::UndoRedoManager::EntityMap.find(m_entity) != bee::UndoRedoManager::EntityMap.end())
        {
            m_entity = bee::UndoRedoManager::EntityMap[m_entity];
        }
        else
        {
            return;
        }
    }
    m_entityData = serializeEntityString<cereal::JSONOutputArchive>(registry, m_entity);
    bee::ecs::DestroyEntity(m_entity, registry);
}

void bee::CreateEntityCommand::OnImGuiRender() { ImGui::Text("Create Entity Command"); }

// ======= DeleteEntityCommand =======
bee::DeleteEntityCommand::DeleteEntityCommand(entt::entity entity) : m_entity(entity) {}

void bee::DeleteEntityCommand::Execute()
{
    auto& registry = bee::Engine.Registry();
    if (!registry.valid(m_entity))
    {
        // check the undo redo mapping to see if the entity was deleted
        if (bee::UndoRedoManager::EntityMap.find(m_entity) != bee::UndoRedoManager::EntityMap.end())
        {
            m_entity = bee::UndoRedoManager::EntityMap[m_entity];
        }
        else
        {
            return;
        }
    }
    m_entityData = serializeEntityString<cereal::JSONOutputArchive>(registry, m_entity);
    bee::ecs::DestroyEntity(m_entity, bee::Engine.Registry());
}

void bee::DeleteEntityCommand::Undo()
{
    auto& registry = bee::Engine.Registry();
    if (registry.valid(m_entity)) return;
    std::unordered_map<entt::entity, entt::entity> entity_mapping;

    deserializeEntityString<cereal::JSONInputArchive>(m_entityData, registry, entity_mapping);
    bee::ecs::UpdateEntityMapping(entity_mapping, registry);

    m_entity = entity_mapping[m_entity];

    // add the entity mapping to the undo redo manager mapping
    for (auto& [oldEntity, newEntity] : entity_mapping)
    {
        bee::UndoRedoManager::EntityMap[oldEntity] = newEntity;
    }
}

void bee::DeleteEntityCommand::OnImGuiRender() { ImGui::Text("Delete Entity Command"); }
