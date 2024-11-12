import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

interface MongoDBProject {
  _id: string | { toString(): string };
  moderator: string;
  project: string;
  members: string[];
  estimated_completion_date: string;
  min_members: { $numberInt: string };
  max_members: { $numberInt: string };
}

interface Project {
  _id: string;
  moderator: string;
  project: string;
  members: string[];
  estimated_completion_date: string;
  min_members: number;
  max_members: number;
}

const Box: React.FC = () => {
  const navigate = useNavigate();
  const [projects, setProjects] = useState<Project[]>([]);
  const [newProject, setNewProject] = useState<Omit<Project, '_id'>>({
    moderator: '',
    project: '',
    members: [''],
    estimated_completion_date: '',
    min_members: 2,
    max_members: 10
  });

  useEffect(() => {
    fetchProjects();
  }, []);

  const fetchProjects = async () => {
    try {
      const response = await fetch('http://localhost:8080/projects');
      const mongoProjects: MongoDBProject[] = await response.json();
      
      const convertedProjects: Project[] = mongoProjects.map(project => {
        const projectId = typeof project._id === 'object' ? project._id.toString() : project._id;
        console.log('Project ID:', projectId);
        return {
          _id: projectId,
          moderator: project.moderator,
          project: project.project,
          members: project.members,
          estimated_completion_date: project.estimated_completion_date,
          min_members: parseInt(project.min_members.$numberInt),
          max_members: parseInt(project.max_members.$numberInt)
        };
      });
      
      setProjects(convertedProjects);
    } catch (error) {
      console.error('Error fetching projects:', error);
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    try {
      const response = await fetch('http://localhost:8080/newproject', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(newProject)
      });

      if (response.ok) {
        setNewProject({
          moderator: '',
          project: '',
          members: [''],
          estimated_completion_date: '',
          min_members: 2,
          max_members: 10
        });
        await fetchProjects();
      }
    } catch (error) {
      console.error('Error creating project:', error);
    }
  };

  const addMember = () => {
    if (newProject.members.length < newProject.max_members) {
      setNewProject({
        ...newProject,
        members: [...newProject.members, '']
      });
    }
  };

  const removeMember = (indexToRemove: number) => {
    if (newProject.members.length > newProject.min_members) {
      setNewProject({
        ...newProject,
        members: newProject.members.filter((_, index) => index !== indexToRemove)
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
    <div style={styles.container}>
      <div style={styles.formSection}>
        <h2>Create New Project</h2>
        <form onSubmit={handleSubmit}>
          <div>
            <label style={styles.label}>
              Moderator:
              <input
                type="text"
                value={newProject.moderator}
                onChange={(e) => setNewProject({ ...newProject, moderator: e.target.value })}
                style={styles.input}
                required
              />
            </label>
          </div>
          <div>
            <label style={styles.label}>
              Project Name:
              <input
                type="text"
                value={newProject.project}
                onChange={(e) => setNewProject({ ...newProject, project: e.target.value })}
                style={styles.input}
                required
              />
            </label>
          </div>
          <div>
            <label style={styles.label}>
              Estimated Completion Date:
              <input
                type="date"
                value={newProject.estimated_completion_date}
                onChange={(e) => setNewProject({ ...newProject, estimated_completion_date: e.target.value })}
                style={styles.input}
                required
              />
            </label>
          </div>
          <div>
            <label style={styles.label}>Minimum Members:</label>
            <input
              type="number"
              value={newProject.min_members}
              onChange={(e) => setNewProject({ 
                ...newProject, 
                min_members: Math.max(1, parseInt(e.target.value) || 1)
              })}
              style={styles.input}
              min="1"
              required
            />
          </div>
          <div>
            <label style={styles.label}>Maximum Members:</label>
            <input
              type="number"
              value={newProject.max_members}
              onChange={(e) => {
                const maxValue = parseInt(e.target.value) || newProject.min_members;
                setNewProject({ 
                  ...newProject, 
                  max_members: Math.max(maxValue, newProject.min_members)
                });
              }}
              style={styles.input}
              min={newProject.min_members}
              required
            />
          </div>
          <div>
            <label style={styles.label}>Members:</label>
            {newProject.members.map((member, index) => (
              <div key={index} style={styles.memberRow}>
                <input
                  type="text"
                  value={member}
                  onChange={(e) => updateMember(index, e.target.value)}
                  style={styles.input}
                  required
                />
                {newProject.members.length > newProject.min_members && (
                  <button
                    type="button"
                    onClick={() => removeMember(index)}
                    style={styles.removeButton}
                  >
                    Remove
                  </button>
                )}
              </div>
            ))}
            {newProject.members.length < newProject.max_members && (
              <button
                type="button"
                onClick={addMember}
                style={styles.addButton}
              >
                Add Member
              </button>
            )}
          </div>
          <button type="submit" style={styles.submitButton}>
            Create Project
          </button>
        </form>
      </div>

      <div style={styles.projectsGrid}>
        {projects.map((project) => (
          <div key={typeof project._id === 'object' ? (project._id as { toString(): string }).toString() : project._id} style={styles.projectCard}>
            <h3 style={styles.cardHeading}>{project.project}</h3>
            <p style={styles.cardText}><strong>Moderator:</strong> {project.moderator}</p>
            <p style={styles.cardText}>
              <strong>Estimated Completion:</strong> 
              {new Date(project.estimated_completion_date).toLocaleDateString()}
            </p>
            <p style={styles.cardText}>
              <strong>Member Limits:</strong> {project.min_members} - {project.max_members}
            </p>
            
            <div>
              <h4>Members:</h4>
              <ul style={styles.membersList}>
                {project.members.map((member, index) => (
                  <li key={`${project._id}-member-${index}`} style={styles.cardText}>
                    {member}
                  </li>
                ))}
              </ul>
              <button 
                onClick={() => {
                  const projectId = typeof project._id === 'object' ? (project._id as { toString(): string }).toString() : project._id;
                  console.log('Project ID:', projectId);
                  navigate(`/edit-members/${projectId}`);
                }}
                style={styles.editButton}
              >
                Edit Members
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};

const styles = {
  container: {
    maxWidth: '1200px',
    margin: '0 auto',
    padding: '20px',
    width: '100%'
  },
  formSection: {
    marginBottom: '40px',
    padding: '30px',
    backgroundColor: 'white',
    borderRadius: '12px',
    boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
    width: '100%',
    maxWidth: '800px',
    textAlign: 'left' as const
  },
  projectsGrid: {
    display: 'grid',
    gridTemplateColumns: 'repeat(3, 1fr)',
    gap: '20px',
    padding: '20px 0'
  },
  projectCard: {
    backgroundColor: 'white',
    padding: '20px',
    borderRadius: '12px',
    boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
    border: '1px solid #ddd',
    display: 'flex',
    flexDirection: 'column' as const,
    height: '100%',
    textAlign: 'left' as const,
    color: '#333'
  },
  cardHeading: {
    margin: '0 0 15px 0',
    fontSize: '1.5rem',
    color: '#2c3e50'
  },
  cardText: {
    margin: '5px 0',
    fontSize: '14px'
  },
  membersList: {
    listStyle: 'none',
    padding: '0',
    margin: '10px 0'
  },
  label: {
    display: 'block',
    marginBottom: '5px',
    color: '#333',
    fontSize: '14px',
    fontWeight: '500' as const
  },
  input: {
    width: '100%',
    padding: '8px 12px',
    margin: '8px 0',
    border: '1px solid #ddd',
    borderRadius: '4px',
    fontSize: '14px'
  },
  memberRow: {
    display: 'flex',
    gap: '10px',
    alignItems: 'center',
    marginBottom: '10px'
  },
  removeButton: {
    backgroundColor: '#dc3545',
    color: 'white',
    padding: '8px 16px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer'
  },
  addButton: {
    backgroundColor: '#28a745',
    color: 'white',
    padding: '8px 16px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    marginTop: '10px'
  },
  submitButton: {
    backgroundColor: '#007bff',
    color: 'white',
    padding: '10px 20px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    marginTop: '20px',
    fontSize: '16px'
  },
  editButton: {
    backgroundColor: '#007bff',
    color: 'white',
    padding: '8px 16px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    marginTop: '10px',
    fontSize: '14px'
  }
};

export default Box;
