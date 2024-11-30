import React, { useState, useEffect } from 'react';
import { useParams, useNavigate } from 'react-router-dom';

const EditProject = () => {
  const { id } = useParams();
  const navigate = useNavigate();
  const [project, setProject] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetchProject = async () => {
      try {
        const response = await fetch(`http://localhost:8081/projects/${id}`);
        if (!response.ok) throw new Error('Failed to fetch project');
        const text = await response.text();
        const data = JSON.parse(text);
        
        // Transform the data if needed
        const transformedData = {
          ...data,
          _id: data._id?.$oid || data._id,
          min_members: Number(data.min_members?.$numberInt || data.min_members),
          max_members: Number(data.max_members?.$numberInt || data.max_members),
          current_member_count: Number(data.current_member_count?.$numberInt || data.current_member_count),
        };
        
        setProject(transformedData);
      } catch (err) {
        console.error('Error details:', err);
        setError(err.message);
      } finally {
        setLoading(false);
      }
    };

    fetchProject();
  }, [id]);

  const handleMemberUpdate = async (newMembers) => {
    try {
      console.log('Sending update request with members:', newMembers); // Debug log
      
      const response = await fetch(`http://localhost:8081/updateproject/${id}`, {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          members: newMembers
        }),
      });

      if (!response.ok) {
        const errorText = await response.text();
        console.error('Server response:', errorText); // Debug log
        throw new Error('Failed to update members');
      }
      
      // Update local state
      setProject(prev => ({
        ...prev,
        members: newMembers,
        current_member_count: newMembers.length
      }));
    } catch (err) {
      console.error('Update error:', err); // Debug log
      setError(err.message);
    }
  };

  const addMember = () => {
    if (project.members.length < project.max_members) {
      const newMembers = [...project.members, ''];
      setProject(prev => ({
        ...prev,
        members: newMembers
      }));
    }
  };

  const removeMember = (index) => {
    if (project.members.length > project.min_members) {
      const newMembers = project.members.filter((_, i) => i !== index);
      handleMemberUpdate(newMembers);
    }
  };

  const updateMember = (index, value) => {
    const newMembers = project.members.map((m, i) => i === index ? value : m);
    setProject(prev => ({
      ...prev,
      members: newMembers
    }));
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    await handleMemberUpdate(project.members);
    navigate('/');
  };

  if (loading) return <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
    <div className="text-white text-xl">Loading project...</div>
  </div>;

  if (error) return <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
    <div className="text-white text-xl">Error: {error}</div>
  </div>;

  if (!project) return null;

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 p-8">
      <div className="max-w-2xl mx-auto bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl border border-white/20">
        <div className="flex justify-between items-center mb-8">
          <h1 className="text-3xl font-bold text-white">Edit Project: {project.project}</h1>
          <button
            onClick={() => navigate('/')}
            className="text-white/80 hover:text-white"
          >
            ← Back
          </button>
        </div>

        <div className="space-y-6">
          <div className="text-white/90">
            <p className="font-semibold">Moderator:</p>
            <p>{project.moderator}</p>
          </div>

          <div className="text-white/90">
            <p className="font-semibold">Member Capacity:</p>
            <p>{project.current_member_count} / {project.max_members} members (Minimum: {project.min_members})</p>
          </div>

          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <label className="block text-white mb-4 font-semibold">Members:</label>
              {project.members.map((member, index) => (
                <div key={index} className="flex gap-2 mb-2">
                  <input
                    type="text"
                    value={member}
                    onChange={(e) => updateMember(index, e.target.value)}
                    className="flex-1 px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                    placeholder="Member name"
                  />
                  {project.members.length > project.min_members && (
                    <button
                      type="button"
                      onClick={() => removeMember(index)}
                      className="px-4 py-2 bg-red-500/20 text-white rounded-lg hover:bg-red-500/30"
                    >
                      Remove
                    </button>
                  )}
                </div>
              ))}
              
              {project.members.length < project.max_members && (
                <button
                  type="button"
                  onClick={addMember}
                  className="mt-2 px-4 py-2 bg-white/20 text-white rounded-lg hover:bg-white/30"
                >
                  Add Member
                </button>
              )}
            </div>

            <div className="flex gap-4">
              <button
                type="submit"
                className="flex-1 px-6 py-3 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 transition-colors"
              >
                Save Changes
              </button>
              <button
                type="button"
                onClick={() => navigate('/')}
                className="px-6 py-3 bg-white/20 text-white rounded-lg hover:bg-white/30 transition-colors"
              >
                Cancel
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default EditProject; 