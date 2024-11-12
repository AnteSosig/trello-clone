import React, { useState, useEffect } from 'react';

interface Project {
  moderator: string;
  project: string;
  members: string[];
  estimated_completion_date: string;
  min_members: number;
  max_members: number;
  current_member_count?: number;  // Optional as it's calculated server-side
}

const Box: React.FC = () => {
  const [projects, setProjects] = useState<Project[]>([]);
  const [newProject, setNewProject] = useState<Project>({
    moderator: '',
    project: '',
    members: [''],
    estimated_completion_date: '',
    min_members: 2,  // Default values
    max_members: 10  // Default values
  });

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    try {
      const response = await fetch('http://localhost:8080/newproject', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json'
        },
        body: JSON.stringify(newProject)
      });

      if (response.ok) {
        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
          const data = await response.json();
          setProjects([...projects, data]);
        } else {
          const text = await response.text();
          console.log('Server response:', text);
        }
        
        setNewProject({
          moderator: '',
          project: '',
          members: [''],
          estimated_completion_date: '',
          min_members: 2,
          max_members: 10
        });
      } else {
        const text = await response.text();
        console.error('Server error:', text);
      }
    } catch (error) {
      console.error('Error creating project:', error);
    }
  };

  const addMember = () => {
    if (newProject.members.length < newProject.max_members) { // Match C backend limit
      setNewProject({
        ...newProject,
        members: [...newProject.members, '']
      });
    }
  };

  const updateMember = (index: number, value: string) => {
    const updatedMembers = [...newProject.members];
    updatedMembers[index] = value;
    setNewProject({
      ...newProject,
      members: updatedMembers
    });
  };

  return (
    <div className="box">
      <h2>Create New Project</h2>
      <form onSubmit={handleSubmit}>
        <div>
          <label>
            Moderator:
            <input
              type="text"
              maxLength={99} // Updated to match new MAX_STRING_LENGTH
              value={newProject.moderator}
              onChange={(e) => setNewProject({
                ...newProject,
                moderator: e.target.value
              })}
              required
            />
          </label>
        </div>

        <div>
          <label>
            Project Name:
            <input
              type="text"
              maxLength={99} // Updated to match new MAX_STRING_LENGTH
              value={newProject.project}
              onChange={(e) => setNewProject({
                ...newProject,
                project: e.target.value
              })}
              required
            />
          </label>
        </div>

        <div>
          <label>
            Estimated Completion Date:
            <input
              type="date"
              value={newProject.estimated_completion_date}
              onChange={(e) => setNewProject({
                ...newProject,
                estimated_completion_date: e.target.value
              })}
              required
            />
          </label>
        </div>

        <div>
          <label>
            Minimum Members:
            <input
              type="number"
              min="1"
              max={newProject.max_members}
              value={newProject.min_members}
              onChange={(e) => setNewProject({
                ...newProject,
                min_members: parseInt(e.target.value)
              })}
              required
            />
          </label>
        </div>

        <div>
          <label>
            Maximum Members:
            <input
              type="number"
              min={newProject.min_members}
              max="50"
              value={newProject.max_members}
              onChange={(e) => setNewProject({
                ...newProject,
                max_members: parseInt(e.target.value)
              })}
              required
            />
          </label>
        </div>

        <div>
          <h3>Members:</h3>
          {newProject.members.map((member, index) => (
            <div key={index}>
              <input
                type="text"
                maxLength={99} // Updated to match new MAX_STRING_LENGTH
                value={member}
                onChange={(e) => updateMember(index, e.target.value)}
                placeholder={`Member ${index + 1}`}
                required={index < newProject.min_members} // Make fields required up to min_members
              />
            </div>
          ))}
          {newProject.members.length < newProject.max_members && (
            <button type="button" onClick={addMember}>
              Add Member
            </button>
          )}
        </div>

        <button type="submit">Create Project</button>
      </form>

      <div>
        <h2>Existing Projects</h2>
        {projects.map((project, index) => (
          <div key={index} className="project-card">
            <h3>{project.project}</h3>
            <p>Moderator: {project.moderator}</p>
            <h4>Members:</h4>
            <ul>
              {project.members.map((member, mIndex) => (
                <li key={mIndex}>{member}</li>
              ))}
            </ul>
          </div>
        ))}
      </div>
    </div>
  );
};

export default Box;
