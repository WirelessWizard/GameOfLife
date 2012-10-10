#include "game_of_life.h"

void GameOfLife::init()
{
    node_x_size_ = new std::map<std::pair<int, int>, int>;
    node_y_size_ = new std::map<std::pair<int, int>, int>;
    redundant_nodes_ = new std::map<int, bool>;
}

GameOfLife::GameOfLife(int X, int Y, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(X, Y);
    
    init();
    generate_random();
	print();
    //linear_split();
    grid_split();
}

GameOfLife::GameOfLife(const Matrix &mat, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(mat);
    	
    init();
    print();
    //linear_split();
    grid_split();
}

GameOfLife::~GameOfLife()
{
    delete field_;
    delete node_x_size_;
    delete node_y_size_;
    delete redundant_nodes_;
}

void GameOfLife::generate_random()
{
    srand(time(0));

    for (int y = 0; y < field_->size_y(); y ++)
    {
    	for (int x = 0; x < field_->size_x(); x ++)
    	{
    		if (rand() % 2) field_->set(x, y, ALIVE);
    		else            field_->set(x, y, DEAD);
    	}
    }
}

void GameOfLife::collect()
{    
    int x_inc = 0, y_inc = 0, rank = 0;

    Matrix *submat = NULL;
        		
    for (int n_y = 0; n_y < n_y_nodes_; n_y ++)
    {
    	for (int n_x = 0; n_x < n_x_nodes_; n_x ++)
    	{
    		rank = n_y * n_x_nodes_ + n_x;
    		
    		if ((*redundant_nodes_)[rank]) continue;
    	    			
    		int x_size = (*node_x_size_)[std::pair<int, int>(n_x, n_y)],
    			y_size = (*node_y_size_)[std::pair<int, int>(n_x, n_y)];
    		
    		{
				LIFE *loc_job = new LIFE[x_size * y_size]();
				
				MPI_Recv(loc_job, x_size * y_size, MPI_INT, rank, MASS + rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
				submat = new Matrix(x_size, y_size, loc_job);
						
				delete loc_job;
    		}
    		
    		printf("rank = %d, n_x = %d, n_y = %d, x_inc = %d, y_inc = %d (x_size = %d, y_size = %d)\n",
    		       rank, n_x, n_y, x_inc, y_inc, x_size, y_size);
    		    						
    		for (int y = 0; y < y_size; y ++)
    		{
    			for (int x = 0; x < x_size; x ++)
    			{
    				field_->set(x + x_inc, y + y_inc, submat->get(x, y));
    			}
    		}
    		    		
    		if ((rank + 1) % n_x_nodes_ == 0)
    		{
    			x_inc = 0;
    			y_inc += y_size;
    		}
    		else
    		{
    			x_inc += x_size;
    		}
    		    		
    		delete submat;
    	}
    }
}

void GameOfLife::send_init_data(int target_rank, int x_pos, int y_pos, int x_size, int y_size, int *neighbors, LIFE *jbuf)
{    
	bool red = (*redundant_nodes_)[target_rank];
	
    //printf("Node#%d is %s\n", target_rank, red ? "redundant!" : "not redundant!");
    
    if (red)
    {
    	MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);
    	return;	
    }
    MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);
    
    MPI_Send(&n_gens_, 1, MPI_INT, target_rank, N_GENERATIONS, MPI_COMM_WORLD);
    MPI_Send(&x_size, 1, MPI_INT, target_rank, NODE_X_SIZE, MPI_COMM_WORLD);
    MPI_Send(&y_size, 1, MPI_INT, target_rank, NODE_Y_SIZE, MPI_COMM_WORLD);
    MPI_Send(neighbors, N_NEIGHBORS, MPI_INT, target_rank, NEIGHBORS, MPI_COMM_WORLD);
    MPI_Send(jbuf, x_size * y_size, MPI_INT, target_rank, MASS, MPI_COMM_WORLD);
}

int GameOfLife::find_neighbor(int my_x, bool right_not_left)
{ // Geekie Coddie (old version)
    int nei = right_not_left ? (my_x + 1) % n_x_nodes_
                             : (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
                    
    if ((*redundant_nodes_)[nei]) return find_neighbor(nei, right_not_left);
    return nei;
}

int GameOfLife::find_neighbor(int my_x, int my_y, NEIGHBOR nei_dir)
{
// Generalized Geekie Coddie
	bool print_debug = false;
    if (my_x == 0 && my_y == 0) print_debug = true; 

    switch (nei_dir)
    {
    	case TOP:
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	case RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		break;
    	case BOTTOM:
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		break;
    	case TOP_RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	case BOTTOM_RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case BOTTOM_LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case TOP_LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	default:
    		printf(">>> Error: wrong neighbor direction sent to find_neighbor function.\n");
    		return -1;
    }
                                                    
	int nei_rank = my_y * n_x_nodes_ + my_x;

    //if (print_debug) printf("nei_rank = %d, nei_dir = %d, my_x = %d, my_y = %d\n", nei_rank, nei_dir, my_x, my_y);
	
    if ((*redundant_nodes_)[nei_rank])
    	return find_neighbor(my_x, my_y, nei_dir);
    
    return nei_rank;
}

void GameOfLife::linear_split()
{
    // Strategy:
    n_x_nodes_ = n_nodes_;
    n_y_nodes_ = 1;

    int workload_per_node = 0;
    int zero_node_workload = 0;
	int n_actual_nodes = n_nodes_;
	        
	// Taking into account the marginal cases:
    if (n_nodes_ > field_->size_x())
    {
    	workload_per_node = (int) ( field_->size_x() / n_nodes_ );
    	zero_node_workload = 0;
    }
    else if (n_nodes_ > 1)
    {
    	workload_per_node = (int) (field_->size_x() / n_nodes_) * (n_nodes_ / (n_nodes_ - 1));
    	zero_node_workload = field_->size_x() - (n_nodes_ - 1) * workload_per_node;
    }
    else
    {
    	workload_per_node = 0;
    	zero_node_workload = field_->size_x();
    }
	if (workload_per_node == 0)
	{
		workload_per_node = 1;
		n_actual_nodes = field_->size_x() - zero_node_workload;
	}
	
	// Filling the Game's hyper torus with 2-cell-wide boarder overlap:
	std::map<int, Matrix *> jbuf;
	
	for (int n = 0; n < n_actual_nodes + 1; n ++)
	{
		int workload = n == 0 ? zero_node_workload : workload_per_node;
			
		if (workload == 0)
		{
	        (*redundant_nodes_)[n] = true;
	        continue;
		}
		
		jbuf[n] = new Matrix(workload + 2, field_->size_y() + 2);
		
		for (int y = -1; y < field_->size_y() + 1; y ++)
		{
			for (int loc_x = 0,
					 x = zero_node_workload + (n - 1) * workload - 1;
					 x < zero_node_workload +  n      * workload + 1;
				 loc_x ++, x ++)
			{        	
				if (field_->get(x>=0? (x < field_->size_x()? x : 0)
									: field_->size_x() - 1,
								y>=0? (y < field_->size_y()? y : 0)
									: field_->size_y() - 1) == ALIVE)
				
					jbuf[n]->set(loc_x, y + 1, ALIVE);
				else
					jbuf[n]->set(loc_x, y + 1, DEAD);
			}
		}
			
		(*node_x_size_)[std::pair<int, int>(n, 0)] = workload;
		(*node_y_size_)[std::pair<int, int>(n, 0)] = field_->size_y();
							
		(*redundant_nodes_)[n] = false;
	}
	
	// Don't forget to init the redundant nodes as well:
	for (int n = n_actual_nodes + 1; n < n_nodes_; n ++)
	{
		(*redundant_nodes_)[n] = true;
	}
	
    // Generate per-node neighbor lists and send all per-node init data:
    for (int n = 0; n < n_nodes_; n ++)
    {
    	if ((*redundant_nodes_)[n])
    	{
	        send_init_data(n, n, 0, 0, 0, NULL, NULL);
	    	continue;    
	    }    

        int *neighbors = new int[N_NEIGHBORS]();
        
		int right_nei = find_neighbor(n, true);
		int left_nei = find_neighbor(n, false);
		        	
		neighbors[   TOP] = n;
		neighbors[BOTTOM] = n;
		
		neighbors[   TOP_RIGHT] = right_nei;
		neighbors[       RIGHT] = right_nei;
		neighbors[BOTTOM_RIGHT] = right_nei;
		
		neighbors[BOTTOM_LEFT] = left_nei;
		neighbors[       LEFT] = left_nei;
		neighbors[   TOP_LEFT] = left_nei;
		
		send_init_data(n, n, 0, (n == 0 ? zero_node_workload : workload_per_node) + 2, field_->size_y() + 2, neighbors, jbuf[n]->data());
	
		delete neighbors;
		delete jbuf[n];
    }
}

void GameOfLife::grid_split()
{
	std::vector<num_t> *prime_factors = get_prime_factors(n_nodes_);
	
	int x_cells_per_node = field_->size_x(),
        y_cells_per_node = field_->size_y();
            
    n_x_nodes_ = 1;
    n_y_nodes_ = 1;
    	
	for (std::vector<num_t>::const_iterator iter = prime_factors->begin(); iter != prime_factors->end(); iter ++)
	{
		if (x_cells_per_node > y_cells_per_node)
		{
			x_cells_per_node /= *iter;
			n_x_nodes_ *= *iter;
		}
		else
		{
			y_cells_per_node /= *iter;
			n_y_nodes_ *= *iter;
		}
	}

	printf("n_nodes_ = %d\n", n_nodes_);
	printf("prime_factors->size() = %d\n", (int) prime_factors->size());	
	printf("x_cells_per_node = %d\n", x_cells_per_node);
	printf("y_cells_per_node = %d\n", y_cells_per_node);
	printf("n_x_nodes_ = %d\n", n_x_nodes_);
	printf("n_y_nodes_ = %d\n", n_y_nodes_);
		
	delete prime_factors;

    int x_extra_job = field_->size_x() - x_cells_per_node * n_x_nodes_,
	    y_extra_job = field_->size_y() - y_cells_per_node * n_y_nodes_;

   	printf("x_extra_job = %d\n", x_extra_job);
	printf("y_extra_job = %d\n", y_extra_job);
	
	int n_actual_nodes = n_nodes_;
	
	std::map<int, Matrix *> jbuf;
	
	for (int n = 0; n < n_actual_nodes; n ++)
	{
		int node_x_coord = n % n_x_nodes_,
		    node_y_coord = (int) n / n_x_nodes_;
	
		int x_workload = x_cells_per_node + (node_x_coord < n_x_nodes_ - 1 ? 0 : x_extra_job),
		    y_workload = y_cells_per_node + (node_y_coord < n_y_nodes_ - 1 ? 0 : y_extra_job);
		    
	   	//printf("x_workload = %d\n", x_workload);
		//printf("y_workload = %d\n", y_workload);
		    
		if (x_workload * y_workload == 0)
		{
	        (*redundant_nodes_)[n] = true;
	        continue;
		}
		
		jbuf[n] = new Matrix(x_workload + 2, y_workload + 2);
		
		//printf("I'm here! (n = %d, {%d %d}) -- workload (%dX%d)\n", n, node_x_coord, node_y_coord, x_workload, y_workload);
				
		for (int loc_y = 0,
				 y = node_y_coord * y_cells_per_node - 1;
				 y < node_y_coord * y_cells_per_node + y_workload + 1;
		     loc_y ++, y ++)
		{
			for (int loc_x = 0,
					 x = node_x_coord * x_cells_per_node - 1;
					 x < node_x_coord * x_cells_per_node + x_workload + 1;
				 loc_x ++, x ++)
			{
				if (field_->get(x>=0? (x < field_->size_x()? x : 0)
									: field_->size_x() - 1,
								y>=0? (y < field_->size_y()? y : 0)
									: field_->size_y() - 1) == ALIVE)
				
					jbuf[n]->set(loc_x, loc_y, ALIVE);
				else
					jbuf[n]->set(loc_x, loc_y, DEAD);
			}
		}
			
		(*node_x_size_)[std::pair<int, int>(node_x_coord, node_y_coord)] = x_workload;
		(*node_y_size_)[std::pair<int, int>(node_x_coord, node_y_coord)] = y_workload;

		(*redundant_nodes_)[n] = false;
	}
	
	// Don't forget to init the redundant nodes as well:
	for (int n = n_actual_nodes; n < n_nodes_; n ++)
	{
		(*redundant_nodes_)[n] = true;
	}
	
    // Generate per-node neighbor lists and send all per-node init data:
    for (int n = 0; n < n_nodes_; n ++)
    {
    	if ((*redundant_nodes_)[n])
    	{
	        send_init_data(n, 0, 0, 0, 0, NULL, NULL);
	    	continue;
	    }

        int *neighbors = new int[N_NEIGHBORS]();		    
		 
		int node_x_coord = n % n_x_nodes_,
		    node_y_coord = (int) n / n_x_nodes_;
		        	
		for (int i = 0; i < N_NEIGHBORS; i ++)
		{
			neighbors[(NEIGHBOR) i] = find_neighbor(node_x_coord, node_y_coord, (NEIGHBOR) i);
		}
		
		jbuf[n]->print();
		
		send_init_data(n, node_x_coord, node_y_coord,
		               2 + x_cells_per_node + (node_x_coord < n_x_nodes_ - 1 ? 0 : x_extra_job),
		               2 + y_cells_per_node + (node_y_coord < n_y_nodes_ - 1 ? 0 : y_extra_job),
		               neighbors, jbuf[n]->data());
	
		delete neighbors;
		delete jbuf[n];
    }	
}
