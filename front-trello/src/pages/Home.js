import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

const Home = () => {
  const [isGridView, setIsGridView] = useState(true);
  const [cards, setCards] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [selectedCard, setSelectedCard] = useState(null);
  const [showNewProjectModal, setShowNewProjectModal] = useState(false);

  const fetchProjects = async () => {
    try {
      setLoading(true);
      const response = await fetch('http://localhost:8080/projects');
      if (!response.ok) {
        throw new Error('Failed to fetch projects');
      }
      const data = await response.json();
      
      const transformedData = data.map(project => ({
        ...project,
        min_members: Number(project.min_members?.$numberInt || project.min_members),
        max_members: Number(project.max_members?.$numberInt || project.max_members),
        current_member_count: Number(project.current_member_count?.$numberInt || project.current_member_count),
      }));
      
      setCards(transformedData);
    } catch (err) {
      console.error('Error fetching projects:', err);
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchProjects();
  }, []);

  if (loading) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
        <div className="text-white text-xl">Loading projects...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
        <div className="text-white text-xl">Error: {error}</div>
      </div>
    );
  }

  const DetailModal = ({ card, onClose }) => {
    const navigate = useNavigate();

    if (!card) return null;

    const projectId = card._id?.$oid || card._id;

    return (
      <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
          <div className="flex justify-between items-start mb-6">
            <h2 className="text-3xl font-bold text-white">{card.project}</h2>
            <div className="flex gap-4">
              <button 
                onClick={() => {
                  onClose();
                  navigate(`/edit-project/${projectId}`);
                }}
                className="text-white/80 hover:text-white px-4 py-2 bg-emerald-600/20 rounded-lg"
              >
                Edit Members
              </button>
              <button 
                onClick={onClose}
                className="text-white/80 hover:text-white"
              >
                ✕
              </button>
            </div>
          </div>

          <div className="space-y-4">
            <div className="text-white/90">
              <p className="font-semibold">Moderator:</p>
              <p>{card.moderator}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Description:</p>
              <p>{card.description}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Estimated Completion:</p>
              <p>{card.estimated_completion_date}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Member Capacity:</p>
              <p>{card.current_member_count} / {card.max_members} members (Minimum required: {card.min_members})</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Members:</p>
              <ul className="list-disc list-inside">
                {card.members.map((member, index) => (
                  <li key={index}>{member}</li>
                ))}
              </ul>
            </div>
          </div>
        </div>
      </div>
    );
  };

  const NewProjectModal = ({ onClose }) => {
    const [formData, setFormData] = useState({
      moderator: '',
      project: '',
      estimated_completion_date: '',
      min_members: 1,
      max_members: 2,
      members: ['']
    });

    const handleSubmit = async (e) => {
      e.preventDefault();
      
      try {
        const response = await fetch('http://localhost:8080/newproject', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            ...formData,
            current_member_count: formData.members.filter(m => m).length
          }),
        });

        if (!response.ok) {
          throw new Error('Failed to create project');
        }

        await fetchProjects();
        onClose();
      } catch (err) {
        console.error('Error creating project:', err);
        alert('Failed to create project');
      }
    };

    const addMemberField = () => {
      if (formData.members.length < formData.max_members) {
        setFormData(prev => ({
          ...prev,
          members: [...prev.members, '']
        }));
      }
    };

    const removeMemberField = (index) => {
      setFormData(prev => ({
        ...prev,
        members: prev.members.filter((_, i) => i !== index)
      }));
    };

    const updateMember = (index, value) => {
      setFormData(prev => ({
        ...prev,
        members: prev.members.map((m, i) => i === index ? value : m)
      }));
    };

    const handleInputChange = (e) => {
      const { name, value } = e.target;
      setFormData(prev => ({
        ...prev,
        [name]: value
      }));
    };

    return (
      <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
          <div className="flex justify-between items-start mb-6">
            <h2 className="text-3xl font-bold text-white">Create New Project</h2>
            <button onClick={onClose} className="text-white/80 hover:text-white">✕</button>
          </div>

          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <label className="block text-white mb-2">Project Name</label>
              <input
                type="text"
                name="project"
                required
                className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                value={formData.project}
                onChange={handleInputChange}
              />
            </div>

            <div>
              <label className="block text-white mb-2">Moderator</label>
              <input
                type="text"
                name="moderator"
                required
                className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                value={formData.moderator}
                onChange={handleInputChange}
              />
            </div>

            <div>
              <label className="block text-white mb-2">Estimated Completion Date</label>
              <input
                type="date"
                name="estimated_completion_date"
                required
                className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                value={formData.estimated_completion_date}
                onChange={handleInputChange}
              />
            </div>

            <div className="flex gap-4">
              <div className="flex-1">
                <label className="block text-white mb-2">Min Members</label>
                <input
                  type="number"
                  name="min_members"
                  required
                  min="1"
                  className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                  value={formData.min_members}
                  onChange={e => handleInputChange({ target: { name: 'min_members', value: parseInt(e.target.value) } })}
                />
              </div>
              <div className="flex-1">
                <label className="block text-white mb-2">Max Members</label>
                <input
                  type="number"
                  name="max_members"
                  required
                  min={formData.min_members}
                  className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                  value={formData.max_members}
                  onChange={e => handleInputChange({ target: { name: 'max_members', value: parseInt(e.target.value) } })}
                />
              </div>
            </div>

            <div>
              <label className="block text-white mb-2">Members</label>
              {formData.members.map((member, index) => (
                <div key={index} className="flex gap-2 mb-2">
                  <input
                    type="text"
                    className="flex-1 px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                    value={member}
                    onChange={(e) => updateMember(index, e.target.value)}
                    placeholder="Member name"
                  />
                  <button
                    type="button"
                    onClick={() => removeMemberField(index)}
                    className="px-4 py-2 bg-red-500/20 text-white rounded-lg hover:bg-red-500/30"
                  >
                    Remove
                  </button>
                </div>
              ))}
              {formData.members.length < formData.max_members && (
                <button
                  type="button"
                  onClick={addMemberField}
                  className="mt-2 px-4 py-2 bg-white/20 text-white rounded-lg hover:bg-white/30"
                >
                  Add Member
                </button>
              )}
            </div>

            <button
              type="submit"
              className="w-full px-6 py-3 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 transition-colors"
            >
              Create Project
            </button>
          </form>
        </div>
      </div>
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700">
      {selectedCard && (
        <DetailModal 
          card={selectedCard} 
          onClose={() => setSelectedCard(null)} 
        />
      )}
      
      {showNewProjectModal && (
        <NewProjectModal onClose={() => setShowNewProjectModal(false)} />
      )}

      <div className="container mx-auto px-4 py-12">
        {/* Header Section */}
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 mb-8 max-w-4xl mx-auto text-center shadow-2xl">
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-4">
            Dobrodosli na Trello Clone
          </h1>
          <p className="text-xl text-white/90 leading-relaxed mb-6">
            Svi projekti za vase prijatelje i gejmere na jednom mestu
          </p>
          
          {/* View Toggle and New Project Buttons */}
          <div className="flex justify-center gap-4">
            <button
              onClick={() => setIsGridView(!isGridView)}
              className="px-6 py-2 bg-white/20 text-white rounded-lg font-medium 
                hover:bg-white/30 transition-colors duration-300"
            >
              {isGridView ? 'Switch to List View' : 'Switch to Grid View'}
            </button>
            
            <button
              onClick={() => setShowNewProjectModal(true)}
              className="px-6 py-2 bg-emerald-600 text-white rounded-lg font-medium 
                hover:bg-emerald-700 transition-colors duration-300"
            >
              New Project
            </button>
          </div>
        </div>

        {/* Cards Container */}
        <div className={`grid gap-6 ${isGridView ? 'grid-cols-1 md:grid-cols-2 lg:grid-cols-3' : 'grid-cols-1'}`}>
          {cards.map((card, index) => (
            <div
              key={index}
              className={`bg-white/10 backdrop-blur-lg rounded-xl p-6 shadow-xl 
                hover:transform hover:scale-105 transition-all duration-300
                border border-white/20 group relative
                ${isGridView ? 'h-48' : 'h-32'}`}
            >
              <div className="h-full flex flex-col">
                {isGridView ? (
                  <>
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.project}
                      </h3>
                      <p className="text-white/80">
                        {card.description}
                      </p>
                      <p className="text-white/80">
                        Members: {card.current_member_count}/{card.max_members}
                      </p>
                    </div>
                    <div className="absolute bottom-6 right-6 opacity-0 group-hover:opacity-100 transition-opacity">
                      <button 
                        onClick={() => setSelectedCard(card)}
                        className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                          hover:bg-white/30 transition-colors duration-300 whitespace-nowrap"
                      >
                        View Details
                      </button>
                    </div>
                  </>
                ) : (
                  <div className="flex justify-between items-start w-full">
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.project}
                      </h3>
                      <p className="text-white/80">
                        Members: {card.current_member_count}/{card.max_members}
                      </p>
                    </div>
                    <div className="opacity-0 group-hover:opacity-100 transition-opacity ml-4">
                      <button 
                        onClick={() => setSelectedCard(card)}
                        className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                          hover:bg-white/30 transition-colors duration-300 whitespace-nowrap"
                      >
                        View Details
                      </button>
                    </div>
                  </div>
                )}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default Home;