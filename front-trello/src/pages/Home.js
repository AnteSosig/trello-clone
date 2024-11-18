import React, { useState } from 'react';

const Home = () => {
  const [isGridView, setIsGridView] = useState(true);

  const cards = [
    { title: 'Projekat 1', description: 'Opis projekta 1' },
    { title: 'Projekat 2', description: 'Opis projekta 2' },
    { title: 'Projekat 3', description: 'Opis projekta 3' },
  ];

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700">
      <div className="container mx-auto px-4 py-12">
        {/* Header Section */}
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 mb-8 max-w-4xl mx-auto text-center shadow-2xl">
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-4">
            Dobrodosli na Trello Clone
          </h1>
          <p className="text-xl text-white/90 leading-relaxed mb-6">
            Svi projekti za vase prijatelje i gejmere na jednom mestu
          </p>
          
          {/* View Toggle Button */}
          <button
            onClick={() => setIsGridView(!isGridView)}
            className="px-6 py-2 bg-white/20 text-white rounded-lg font-medium 
              hover:bg-white/30 transition-colors duration-300 flex items-center gap-2 mx-auto"
          >
            {isGridView ? (
              <>
                <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6h16M4 10h16M4 14h16M4 18h16" />
                </svg>
                Switch to List View
              </>
            ) : (
              <>
                <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z" />
                </svg>
                Switch to Grid View
              </>
            )}
          </button>
        </div>

        {/* Cards Container */}
        <div className={`grid gap-6 ${
          isGridView ? 'grid-cols-1 md:grid-cols-2 lg:grid-cols-3' : 'grid-cols-1'
        }`}>
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
                  // Grid View Layout
                  <>
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.title}
                      </h3>
                      <p className="text-white/80">
                        {card.description}
                      </p>
                    </div>
                    <div className="absolute bottom-6 right-6 opacity-0 group-hover:opacity-100 transition-opacity">
                      <button className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                        hover:bg-white/30 transition-colors duration-300 whitespace-nowrap">
                        View Details
                      </button>
                    </div>
                  </>
                ) : (
                  // List View Layout
                  <div className="flex justify-between items-start w-full">
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.title}
                      </h3>
                      <p className="text-white/80">
                        {card.description}
                      </p>
                    </div>
                    <div className="opacity-0 group-hover:opacity-100 transition-opacity ml-4">
                      <button className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                        hover:bg-white/30 transition-colors duration-300 whitespace-nowrap">
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