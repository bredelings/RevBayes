#include "Clade.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RbConstants.h"
#include "RbMathCombinatorialFunctions.h"
#include "StochasticNode.h"
#include "TopologyNode.h"
#include "TreeUtilities.h"
#include "UltrametricTreeDistribution.h"

#include <algorithm>
#include <cmath>


#ifdef RB_MPI
#include <mpi.h>
#endif

using namespace RevBayesCore;

UltrametricTreeDistribution::UltrametricTreeDistribution( TypedDistribution<Tree>* tp, TypedDistribution<double>* rp, TypedDagNode<double> *ra, const TraceTree &tree_trace) : TypedDistribution<Tree>( new Tree() ),
    tree_prior( tp ),
    rate_prior( rp ),
    root_age( ra ),
    num_samples( 0 ),
    sample_block_start( 0 ),
    sample_block_end( num_samples ),
    sample_block_size( num_samples ),
    ln_probs(num_samples, 0.0)
{
    
    int burnin = tree_trace.getBurnin();
    long n_samples = tree_trace.size();
    for (int i=burnin+1; i<n_samples; ++i)
    {
        trees.push_back( tree_trace.objectAt( i ) );
    }
    
    // add the parameters to our set (in the base class)
    // in that way other class can easily access the set of our parameters
    // this will also ensure that the parameters are not getting deleted before we do
    this->addParameter( root_age );
    
    
    // add the parameters of the distribution
    const std::vector<const DagNode*>& pars_tp = tree_prior->getParameters();
    for (std::vector<const DagNode*>::const_iterator it = pars_tp.begin(); it != pars_tp.end(); ++it)
    {
        this->addParameter( *it );
    }
    
    const std::vector<const DagNode*>& pars_rp = rate_prior->getParameters();
    for (std::vector<const DagNode*>::const_iterator it = pars_rp.begin(); it != pars_rp.end(); ++it)
    {
        this->addParameter( *it );
    }
    
    
    
    num_samples = trees.size();
    
    
    sample_block_start = 0;
    sample_block_end   = num_samples;
#ifdef RB_MPI
    sample_block_start = size_t(floor( (double(this->pid - this->active_PID)   / this->num_processes ) * num_samples) );
    sample_block_end   = size_t(floor( (double(this->pid+1 - this->active_PID) / this->num_processes ) * num_samples) );
#endif
    sample_block_size  = sample_block_end - sample_block_start;
    
#ifdef RB_MPI
    // now we need to populate how is
    pid_per_sample = std::vector<size_t>( num_samples, 0 );
    for (size_t i = 0; i < num_samples; ++i)
    {
        pid_per_sample[i] = size_t(floor( double(i) / num_samples * this->num_processes ) ) + this->active_PID;
    }
#endif
    
    ln_probs = std::vector<double>(num_samples, 0.0);
    
    simulateTree();
    
}


UltrametricTreeDistribution::UltrametricTreeDistribution( const UltrametricTreeDistribution &d ) : TypedDistribution<Tree>( d ),
    tree_prior( d.tree_prior->clone() ),
    rate_prior( d.rate_prior->clone() ),
    root_age( d.root_age ),
    trees( d.trees ),
    num_samples( d.num_samples ),
    sample_block_start( d.sample_block_start ),
    sample_block_end( d.sample_block_end ),
    sample_block_size( d.sample_block_size ),
    ln_probs( d.ln_probs )
{
    
}


UltrametricTreeDistribution::~UltrametricTreeDistribution()
{
    
    delete tree_prior;
    delete rate_prior;
}





UltrametricTreeDistribution& UltrametricTreeDistribution::operator=( const UltrametricTreeDistribution &d )
{
    
    if ( this != &d )
    {
        TypedDistribution<Tree>::operator=( d );
        
        // free memory
        delete tree_prior;
        delete rate_prior;
        
        tree_prior          = d.tree_prior->clone();
        rate_prior          = d.rate_prior->clone();
        root_age            = d.root_age;
        trees               = d.trees;
        num_samples         = d.num_samples;
        sample_block_start  = d.sample_block_start;
        sample_block_end    = d.sample_block_end;
        sample_block_size   = d.sample_block_size;
        ln_probs            = d.ln_probs;
        
    }
    
    return *this;
}


///**
// * Recursive call to attach ordered interior node times to the time tree psi. Call it initially with the
// * root of the tree.
// */
//void UltrametricTreeDistribution::attachTimes(Tree* psi, std::vector<TopologyNode *> &nodes, size_t index, const std::vector<double> &interiorNodeTimes, double originTime )
//{
//
//    if (index < num_taxa-1)
//    {
//        // Get the rng
//        RandomNumberGenerator* rng = GLOBAL_RNG;
//
//        // Randomly draw one node from the list of nodes
//        size_t node_index = static_cast<size_t>( floor(rng->uniform01()*nodes.size()) );
//
//        // Get the node from the list
//        TopologyNode* parent = nodes.at(node_index);
//        psi->getNode( parent->getIndex() ).setAge( originTime - interiorNodeTimes[index] );
//
//        // Remove the randomly drawn node from the list
//        nodes.erase(nodes.begin()+long(node_index));
//
//        // Add the left child if an interior node
//        TopologyNode* leftChild = &parent->getChild(0);
//        if ( !leftChild->isTip() )
//        {
//            nodes.push_back(leftChild);
//        }
//
//        // Add the right child if an interior node
//        TopologyNode* rightChild = &parent->getChild(1);
//        if ( !rightChild->isTip() )
//        {
//            nodes.push_back(rightChild);
//        }
//
//        // Recursive call to this function
//        attachTimes(psi, nodes, index+1, interiorNodeTimes, originTime);
//    }
//
//}


///** Build random binary tree to size num_taxa. The result is a draw from the uniform distribution on histories. */
//void UltrametricTreeDistribution::buildRandomBinaryHistory(std::vector<TopologyNode*> &tips)
//{
//
//    if (tips.size() < num_taxa)
//    {
//        // Get the rng
//        RandomNumberGenerator* rng = GLOBAL_RNG;
//
//        // Randomly draw one node from the list of tips
//        size_t index = static_cast<size_t>( floor(rng->uniform01()*tips.size()) );
//
//        // Get the node from the list
//        TopologyNode* parent = tips.at(index);
//
//        // Remove the randomly drawn node from the list
//        tips.erase(tips.begin()+long(index));
//
//        // Add a left child
//        TopologyNode* leftChild = new TopologyNode(0);
//        parent->addChild(leftChild);
//        leftChild->setParent(parent);
//        tips.push_back(leftChild);
//
//        // Add a right child
//        TopologyNode* rightChild = new TopologyNode(0);
//        parent->addChild(rightChild);
//        rightChild->setParent(parent);
//        tips.push_back(rightChild);
//
//        // Recursive call to this function
//        buildRandomBinaryHistory(tips);
//    }
//}


/* Clone function */
UltrametricTreeDistribution* UltrametricTreeDistribution::clone( void ) const
{
    
    return new UltrametricTreeDistribution( *this );
}


double UltrametricTreeDistribution::computeBranchRateLnProbability(const Tree &my_tree, const Tree &sampled_tree)
{
    
    // we need to check if the "outgroup" is present first
    // by "outgroup" I mean the left subtree of the rooted tree.
    const TopologyNode &outgroup = my_tree.getRoot().getChild(0);
    if ( sampled_tree.containsClade(outgroup, true) == false )
    {
        return RbConstants::Double::neginf;
    }
    
    Tree *current_copy = sampled_tree.clone();
    current_copy->reroot( outgroup.getClade(), true );
    
    // initialize the probability
    double ln_prob = RbConstants::Double::neginf;
    
    // first we check if the tree topologies are the same
    if ( my_tree.hasSameTopology( *current_copy ) )
    {
        // reset the probability
        ln_prob = 0.0;
        
        const std::vector<TopologyNode*> &nodes = my_tree.getNodes();
        for (size_t i=0; i<nodes.size(); ++i)
        {
            TopologyNode* the_node = nodes[i];
            if (the_node->isRoot() == false)
            {
                
                const TopologyNode &sampled_node = current_copy->getMrca( *the_node );
                
                double branch_time = the_node->getBranchLength();
                double branch_exp_num_events = sampled_node.getBranchLength();
                double branch_rate = branch_exp_num_events / branch_time;
                
                if ( RbMath::isFinite( branch_rate ) == false )
                    std::cerr << "Rate = " << branch_rate << ",\t\ttime" << branch_time << std::endl;

                
                rate_prior->setValue( new double(branch_rate) );
                ln_prob += rate_prior->computeLnProbability();
//                std::cerr << "P(Rate = " << branch_rate << ") = " << rate_prior->computeLnProbability() << std::endl;
                
            }
        }
    }
    
    delete current_copy;
    
    return ln_prob;
}



/* Compute probability */
double UltrametricTreeDistribution::computeLnProbability( void )
{
    
    size_t num_samples = trees.size();

    // create a temporary copy of the this tree
    Tree *my_tree = value->clone();
    
    // get the root node because we need to make this tree unrooted (for topology comparison)
    TopologyNode *old_root = &my_tree->getRoot();
    size_t child_index = 0;
    if ( old_root->getChild(child_index).isTip() == true )
    {
        child_index = 1;
    }
    TopologyNode *new_root = &old_root->getChild( child_index );
    TopologyNode *second_child = &old_root->getChild( (child_index == 0 ? 1 : 0) );
    
    double bl_first = new_root->getBranchLength();
    double bl_second = second_child->getBranchLength();
    
    old_root->removeChild( new_root );
    old_root->removeChild( second_child );
    new_root->setParent( NULL );
    new_root->addChild( second_child );
    second_child->setParent( new_root );
    
    second_child->setBranchLength( bl_first + bl_second );
    
    // finally we need to set the new root to our tree copy
    my_tree->setRoot( new_root, true);
    my_tree->setRooted( false );
    
    // Variable declarations and initialization
    double ln_prob = 0.0;
    double prob    = 0;
    
    std::vector<double> probs    = std::vector<double>(num_samples, 0.0);
    std::vector<double> ln_probs = std::vector<double>(num_samples, 0.0);
    // add the ln-probs for each sample
    for (size_t i = 0; i < num_samples; ++i)
    {
        if ( i >= sample_block_start && i < sample_block_end )
        {
            const Tree &this_tree = trees[i];
            ln_probs[i] = computeBranchRateLnProbability( *my_tree, this_tree );
        }
        
    }
    
    delete my_tree;
    
#ifdef RB_MPI
    for (size_t i = 0; i < num_samples; ++i)
    {
        
        if ( this->pid == pid_per_sample[i] )
        {
            
            // send the likelihood from the helpers to the master
            if ( this->process_active == false )
            {
                // send from the workers the log-likelihood to the master
                MPI_Send(&ln_probs[i], 1, MPI_DOUBLE, this->active_PID, 0, MPI_COMM_WORLD);
            }
            
        }
        // receive the likelihoods from the helpers
        else if ( this->process_active == true )
        {
            MPI_Status status;
            MPI_Recv(&ln_probs[i], 1, MPI_DOUBLE, pid_per_sample[i], 0, MPI_COMM_WORLD, &status);
        }
        
    }
#endif
    
    
    double max = 0;
    // add the ln-probs for each sample
    for (size_t i = 0; i < num_samples; ++i)
    {
        
#ifdef RB_MPI
        if ( this->process_active == true )
        {
#endif
            if ( i == 0 || max < ln_probs[i] )
            {
                max = ln_probs[i];
            }
            
#ifdef RB_MPI
        }
#endif
        
    }
    
    
#ifdef RB_MPI
    if ( this->process_active == true )
    {
#endif
        
        // now normalize
        for (size_t i = 0; i < num_samples; ++i)
        {
            probs[i] = exp( ln_probs[i] - max);
            prob += probs[i];
        }
        
        ln_prob = std::log( prob ) + max - std::log( num_samples );
        
#ifdef RB_MPI
        
        for (size_t i=this->active_PID+1; i<this->active_PID+this->num_processes; ++i)
        {
            MPI_Send(&ln_prob, 1, MPI_DOUBLE, int(i), 0, MPI_COMM_WORLD);
        }
        
    }
    else
    {
        
        MPI_Status status;
        MPI_Recv(&ln_prob, 1, MPI_DOUBLE, this->active_PID, 0, MPI_COMM_WORLD, &status);
        
    }
#endif
    
    // finally we add the tree prior
    tree_prior->setValue( value->clone() );
    ln_prob += tree_prior->computeLnProbability();
    
    return ln_prob;

}


void UltrametricTreeDistribution::executeMethod(const std::string &n, const std::vector<const DagNode *> &args, RbVector<double> &rv) const
{
    
    if ( n == "getSampleProbabilities" )
    {
        
        bool log_transorm = static_cast<const TypedDagNode<Boolean>* >( args[0] )->getValue();
        
        rv.clear();
        rv.resize(num_samples);
        for (size_t i = 0; i < num_samples; ++i)
        {
            rv[i] = (log_transorm ? ln_probs[i] : exp(ln_probs[i]));
        }
    }
    else
    {
        throw RbException("An empirical-sample distribution does not have a member method called '" + n + "'.");
    }
    
}


void UltrametricTreeDistribution::redrawValue( void )
{
    simulateTree();
}

/** Simulate the tree conditioned on the time of origin */
void UltrametricTreeDistribution::simulateTree( void )
{
    
    delete value;
    
    // Get the rng
    RandomNumberGenerator* rng = GLOBAL_RNG;
    
    size_t index = size_t( rng->uniform01() * trees.size() ) ;
    
    value = trees[index].clone();
    TopologyNode *new_root = new TopologyNode( value->getNumberOfNodes()+1 );
    
    TopologyNode &old_root = value->getRoot();
    TopologyNode &og = old_root.getChild(2);
    old_root.removeChild(&og);
    new_root->addChild(&old_root);
    new_root->addChild( &og );
    old_root.setParent( new_root );
    og.setParent( new_root );
    
    double midpoint = og.getBranchLength() / 2.0;
    old_root.setBranchLength( midpoint );
    og.setBranchLength( midpoint );
    
    value->setRoot( new_root, true);
    value->setRooted( true );
    
    TreeUtilities::makeUltrametric(value);
    TreeUtilities::rescaleTree(value, &(value->getRoot()), root_age->getValue()/value->getRoot().getAge());
    
}

void UltrametricTreeDistribution::getAffected(RbOrderedSet<DagNode *> &affected, RevBayesCore::DagNode *affecter)
{
    
    if ( affecter == root_age)
    {
        dag_node->getAffectedNodes( affected );
    }
    
}

/**
 * Keep the current value and reset some internal flags. Nothing to do here.
 */
void UltrametricTreeDistribution::keepSpecialization(DagNode *affecter)
{
    
    if ( affecter == root_age )
    {
        dag_node->keepAffected();
    }
    
}

/**
 * Restore the current value and reset some internal flags.
 * If the root age variable has been restored, then we need to change the root age of the tree too.
 */
void UltrametricTreeDistribution::restoreSpecialization(DagNode *affecter)
{
    
    if ( affecter == root_age )
    {
        value->getNode( value->getRoot().getIndex() ).setAge( root_age->getValue() );
        dag_node->restoreAffected();
    }
    
}

/** Swap a parameter of the distribution */
void UltrametricTreeDistribution::swapParameterInternal( const DagNode *oldP, const DagNode *newP )
{
    if (oldP == root_age)
    {
        root_age = static_cast<const TypedDagNode<double>* >( newP );
    }
    
}

/**
 * Touch the current value and reset some internal flags.
 * If the root age variable has been restored, then we need to change the root age of the tree too.
 */
void UltrametricTreeDistribution::touchSpecialization(DagNode *affecter, bool touchAll)
{
    
    if ( affecter == root_age )
    {
        value->getNode( value->getRoot().getIndex() ).setAge( root_age->getValue() );
        dag_node->touchAffected();
    }
    
}

