//
// Created by acsr on 4/5/21.
//

#ifndef TRANSITIONMATRIXROUTE_TRANSITION_MATRIX_CONSTRUCTOR_HPP
#define TRANSITIONMATRIXROUTE_TRANSITION_MATRIX_CONSTRUCTOR_HPP

#include <iostream>
#include <iomanip>
#include <fstream>
#include "utility.hpp"
#include "node.hpp"

namespace ACSR {

    class TransitionMatrixConstructor {

    public:
        TransitionMatrixConstructor() = default;

        virtual ~TransitionMatrixConstructor() = default;

        TransitionMatrixConstructor(const TransitionMatrixConstructor &) = delete;

        TransitionMatrixConstructor &operator=(const TransitionMatrixConstructor &) = delete;

        void constructMatrix(int n_wire, const std::string &file_name) {
            std::cout << "start to construct control matrix:\n\tNanowires: " << n_wire << std::endl;
            std::vector<Eigen::Triplet<int, IndexType>> data;
            IndexType dimension = 1;
            for (auto i = 0; i < n_wire; ++i)
                dimension = dimension * 16;

            for (auto index_col = 0; index_col < dimension; ++index_col) {
                auto current_electrode_position = indexToElectrodeVector(n_wire, index_col);
                auto new_electrode_positions = exploreHelper(current_electrode_position);
                //new_electrode_positions.erase(std::remove(new_electrode_positions.begin(),new_electrode_positions.end(),current_electrode_position),new_electrode_positions.end());
                for (auto &new_electrode_position : new_electrode_positions) {
                    auto control = getElectrodesControl(n_wire, current_electrode_position, new_electrode_position);
                    if (control != 0)
                        data.emplace_back(Eigen::Triplet<int, IndexType>(
                                electrodeVectorToIndex(n_wire, new_electrode_position),
                                index_col,
                                1));
                }
            }
            Eigen::SparseMatrix<int, Eigen::ColMajor, IndexType> sparseMatrix;
            sparseMatrix.resize(dimension, dimension);
            sparseMatrix.setFromTriplets(data.begin(), data.end());
            sparseMatrix.makeCompressed();

            std::cout << "Matrix Size: " << sparseMatrix.size() << "\n";
            std::cout << "Matrix NonZeros: " << sparseMatrix.nonZeros() << "\n";
            writeSparseMatrixToBin(file_name, sparseMatrix);
            std::cout << "Transition Matrix Construct Finished!\n Save to " << file_name << "\n";

        }

        void constructControlMatrix(int n_wire, const std::string &file_name) {
            std::cout << "start to construct:\n\tNanowires: " << n_wire << std::endl;
            std::vector<Eigen::Triplet<TransitionControlType, IndexType>> control_data;
            IndexType dimension = 1;
            for (auto i = 0; i < n_wire; ++i)
                dimension = dimension * 16;
            for (auto index_col = 0; index_col < dimension; ++index_col) {
                auto current_electrode_position = indexToElectrodeVector(n_wire, index_col);
                auto new_electrode_positions = exploreHelper(current_electrode_position);
                //new_electrode_positions.erase(std::remove(new_electrode_positions.begin(),new_electrode_positions.end(),current_electrode_position),new_electrode_positions.end());
                for (auto &new_electrode_position : new_electrode_positions) {
                    auto control = getElectrodesControl(n_wire, current_electrode_position, new_electrode_position);
                    if (control != 0)
                        control_data.emplace_back(Eigen::Triplet<TransitionControlType, IndexType>(
                                electrodeVectorToIndex(n_wire, new_electrode_position),
                                index_col,
                                TransitionControlType(control)));
                }
            }
            Eigen::SparseMatrix<TransitionControlType, Eigen::ColMajor, IndexType> sparseMatrix;
            sparseMatrix.resize(dimension, dimension);
            sparseMatrix.setFromTriplets(control_data.begin(), control_data.end());
            sparseMatrix.makeCompressed();

            std::cout << "Control Matrix Size: " << sparseMatrix.size() << "\n";
            std::cout << "Control Matrix NonZeros: " << sparseMatrix.nonZeros() << "\n";

            writeSparseMatrixToBin(file_name, sparseMatrix);

            std::cout << "Transition Control Matrix Construct Finished!\n Save to " << file_name << "\n";
        }

        static void readControlMatrix(const std::string &file_name,
                                      Eigen::SparseMatrix<TransitionControlType, Eigen::ColMajor, IndexType> &control_matrix) {
            readSparsMatrixFromBin(file_name, control_matrix);
        }

        static void readTransitionMatrix(const std::string &file_name,
                                         Eigen::SparseMatrix<int, Eigen::ColMajor, IndexType> &control_matrix) {
            readSparsMatrixFromBin(file_name, control_matrix);
        }


    private:
        const std::vector<SteerDirection> steer_directions = {SteerDirection::Up, SteerDirection::Down,
                                                              SteerDirection::Left, SteerDirection::Right,
                                                              SteerDirection::Stay};


        std::vector<NanowirePositionType> exploreHelper(const NanowirePositionType &states) {
            if (states.empty())
                return std::vector<NanowirePositionType>();
            std::vector<NanowirePositionType> child;

            if (states.size() == 1) {
                for (auto it = steer_directions.begin(); it != steer_directions.end(); ++it) {
                    std::pair<int, int> new_pt;
                    if (steer(states[0], *it, new_pt))
                        child.push_back({new_pt});
                }
            } else {
                NanowirePositionType s(states.begin(), states.end() - 1);
                auto new_child = exploreHelper(s);
                for (auto it = steer_directions.begin(); it != steer_directions.end(); ++it) {

                    std::pair<int, int> new_pt;
                    if (steer(states.back(), *it, new_pt)) {
                        for (auto it1 = new_child.begin(); it1 != new_child.end(); ++it1) {
                            auto c = *it1;
                            c.emplace_back(new_pt);
                            if (validMove(states, c, states.size()))
                                child.push_back(c);
                        }
                    }
                }
            }
            return child;
        }

        static bool
        steer(const std::pair<int, int> &state, SteerDirection direction, std::pair<int, int> &new_position) {
            new_position = state;
            switch (direction) {
                case SteerDirection::Up:
                    new_position.first += 1;
                    break;
                case SteerDirection::Down:
                    new_position.first -= 1;
                    break;
                case SteerDirection::Left:
                    new_position.second -= 1;
                    break;
                case SteerDirection::Right:
                    new_position.second += 1;
                    break;
                default:
                    break;
            }
            if (new_position.first < 0 || new_position.first >= 4)
                return false;
            if (new_position.second < 0 || new_position.second >= 4) {
                return false;
            }
            return true;
        }


        static bool validMove(const NanowirePositionType &state1, const NanowirePositionType &state2, int size) {
            if (state1 == state2)return true;

            for (uint i = 0; i < size; ++i) {
                if (abs((state1[i].first - state2[i].first) + (state1[i].second - state2[i].second)) > 1)return false;
                for (uint j = i + 1; j < size; ++j) {

                    //if more than one nanowire on a same electrode, the should both move on or both stay
                    if (state1[i] == state1[j]) {
                        if ((state1[i] == state2[i]) ^ (state1[j] == state2[j])) {
                            return false;
                        }
                    }
                    //if a nanowire towards a electrode where another nanowire was,
                    if (state2[i] == state1[j] && state2[j] != state1[j])
                        return false;
                    if (state2[j] == state1[i] && state1[i] != state2[i])
                        return false;

                }
            }
            return true;
        }


    };

}

#endif //TRANSITIONMATRIXROUTE_TRANSITION_MATRIX_CONSTRUCTOR_HPP