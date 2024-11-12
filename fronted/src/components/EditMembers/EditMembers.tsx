import React, { useState, useEffect } from 'react';
import { useNavigate, useParams } from 'react-router-dom';

interface MongoDBProject {
  _id: string;
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

const styles = {
  container: {
    padding: '20px',
    maxWidth: '600px',
    margin: '0 auto'
  },
  header: {
    marginBottom: '20px'
  },
  memberRow: {
    display: 'flex',
    gap: '10px',
    marginBottom: '10px',
    alignItems: 'center'
  },
  input: {
    flex: 1,
    padding: '8px',
    borderRadius: '4px',
    border: '1px solid #ddd'
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
  saveButton: {
    backgroundColor: '#007bff',
    color: 'white',
    padding: '10px 20px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    marginTop: '20px'
  },
  cancelButton: {
    backgroundColor: '#6c757d',
    color: 'white',
    padding: '10px 20px',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    marginTop: '20px',
    marginLeft: '10px'
  },
  buttonContainer: {
    display: 'flex',
    gap: '10px',
    marginTop: '20px'
  },
  limits: {
    color: '#666',
    marginBottom: '20px'
  }
};

const EditMembers: React.FC = () => {
  const { projectId } = useParams<{ projectId: string }>();
  const navigate = useNavigate();
  const [project, setProject] = useState<Project | null>(null);
  const [members, setMembers] = useState<string[]>([]);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    const fetchProject = async () => {
      if (!projectId) {
        console.error('No project ID provided');
        setIsLoading(false);
        return;
      }

      try {
        console.log('Fetching project with ID:', projectId);
        const response = await fetch(`http://localhost:8080/project/${projectId}`);
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }

        const mongoProject: MongoDBProject = await response.json();
        
        if (mongoProject) {
          const convertedProject: Project = {
            _id: mongoProject._id,
            moderator: mongoProject.moderator,
            project: mongoProject.project,
            members: mongoProject.members,
            estimated_completion_date: mongoProject.estimated_completion_date,
            min_members: parseInt(mongoProject.min_members.$numberInt),
            max_members: parseInt(mongoProject.max_members.$numberInt)
          };
          
          setProject(convertedProject);
          setMembers(mongoProject.members);
        } else {
          console.error('Project not found with ID:', projectId);
        }
        setIsLoading(false);
      } catch (error) {
        console.error('Error fetching project:', error);
        setIsLoading(false);
      }
    };

    fetchProject();
  }, [projectId]);

  const addMember = () => {
    if (project && members.length < project.max_members) {
      setMembers([...members, '']);
    }
  };

  const removeMember = (index: number) => {
    if (project && members.length > project.min_members) {
      setMembers(members.filter((_, i) => i !== index));
    }
  };

  const updateMember = (index: number, value: string) => {
    const newMembers = [...members];
    newMembers[index] = value;
    setMembers(newMembers);
  };

  const handleSave = async () => {
    if (!project || !projectId) return;

    try {
      const response = await fetch(`http://localhost:8080/updateproject/${projectId}`, {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ members })
      });

      if (response.ok) {
        navigate('/');
      } else {
        console.error('Failed to update members');
      }
    } catch (error) {
      console.error('Error updating members:', error);
    }
  };

  if (isLoading) {
    return <div>Loading...</div>;
  }

  if (!project) {
    return <div>Project not found</div>;
  }

  return (
    <div style={styles.container}>
      <div style={styles.header}>
        <h2>Edit Members - {project.project}</h2>
        <p style={styles.limits}>
          Member limits: {project.min_members} - {project.max_members}
        </p>
      </div>

      {members.map((member, index) => (
        <div key={index} style={styles.memberRow}>
          <input
            type="text"
            value={member}
            onChange={(e) => updateMember(index, e.target.value)}
            style={styles.input}
            placeholder="Enter member name"
          />
          <button
            onClick={() => removeMember(index)}
            style={styles.removeButton}
            disabled={members.length <= project.min_members}
          >
            Remove
          </button>
        </div>
      ))}

      {members.length < project.max_members && (
        <button onClick={addMember} style={styles.addButton}>
          Add Member
        </button>
      )}

      <div style={styles.buttonContainer}>
        <button onClick={handleSave} style={styles.saveButton}>
          Save Changes
        </button>
        <button onClick={() => navigate('/')} style={styles.cancelButton}>
          Cancel
        </button>
      </div>
    </div>
  );
};

export default EditMembers; 