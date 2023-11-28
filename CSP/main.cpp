#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <functional>
#include <unordered_set>

struct Teacher {
    std::string name;
    std::vector<bool> subjectKnowledge;
    int maxHours;
};

struct Group {
    std::string name;
    std::vector<bool> subjects;
    int maxHours;
};

struct Timeslot {
    int subjectIdx = -1;
    int teacherIdx = -1;
    int groupIdx = -1;
};

std::vector<std::string> subjects = {
    "Discrete Math", "Physics", "Programming", "Chemistry", "Philosophy", "Calculus"
};

std::vector<Teacher> teachers = {
    {"Tom", {false, true, false, false, false, true}, 3},
    {"Bob", {false, false, true, true, true, false}, 5},
    {"Mary", {true, false, false, false, true, true}, 8}
};

std::vector<Group> groups = {
    {"A", {false, false, false, false, true, false}, 5},
    {"B", {false, true, false, false, false, true}, 6},
    {"C", {true, false, false, false, false, false}, 2},
    {"D", {false, false, true, false, false, false}, 2},
    {"E", {false, false, false, true, false, false}, 4}
};

size_t numTeachers = teachers.size();
size_t numGroups = groups.size();
size_t numSubjects = subjects.size();
size_t numTimeslots = 15;

class Schedule {
public:
    Schedule() {
        timetable.resize(numTimeslots);
    }

    std::vector<Timeslot> timetable;
};

class CSPSolver {
public:
    CSPSolver() {
        initializeDomains();
        initializeConstraints();
    }

    void solve() {
        backtracking(0);
    }

    void printSchedule(const Schedule &schedule) const {
        std::cout << "Schedule:\n";
        for (size_t timeslotIdx = 0; timeslotIdx < schedule.timetable.size(); ++timeslotIdx) {
            int subjectIdx = schedule.timetable[timeslotIdx].subjectIdx;
            int teacherIdx = schedule.timetable[timeslotIdx].teacherIdx;
            int groupIdx = schedule.timetable[timeslotIdx].groupIdx;

            std::cout << "Timeslot: " << timeslotIdx + 1 << ": ";
            std::cout << "Subject: " << subjects[subjectIdx] << ", ";
            std::cout << "Teacher: " << teachers[teacherIdx].name << ", ";
            std::cout << "Group: " << groups[groupIdx].name << std::endl;
        }
    }

public:
    std::vector<std::vector<int>> domains;
    std::vector<std::function<bool(const Schedule&)>> constraints;
    Schedule bestSchedule, schedule;

    const Schedule& getBestSchedule() const {
        return bestSchedule;
    }

    void initializeDomains() {
        domains.resize(numTimeslots);

        for (size_t timeslotIdx = 0; timeslotIdx < numTimeslots; ++timeslotIdx) {
            domains[timeslotIdx] = generateAssignment(timeslotIdx);
        }
    }

    std::vector<int> generateAssignment(size_t timeslotIdx) {
        std::vector<int> validAssignments;

        for (int subjectIdx = 0; subjectIdx < numSubjects; ++subjectIdx) {
            for (int teacherIdx = 0; teacherIdx < numTeachers; ++teacherIdx) {
                for (int groupIdx = 0; groupIdx < numGroups; ++groupIdx) {
                    int assignment = (int)(subjectIdx + numSubjects * teacherIdx + numSubjects * numTeachers * groupIdx);
                    validAssignments.push_back(assignment);
                }
            }
        }

        return validAssignments;
    }

    bool isValidAssignment(size_t timeslotIdx, int subjectIdx, int teacherIdx, int groupIdx) {

        if (!teachers[teacherIdx].subjectKnowledge[subjectIdx]) {
            return false;
        }

        if (!groups[groupIdx].subjects[subjectIdx]) {
            return false;
        }

        return true;
    }

    void initializeConstraints() {
        constraints.push_back([this](const Schedule& schedule) {
            return noTeacherOverload(schedule) && noGroupOverload(schedule);
        });
    }

    bool noTeacherOverload(const Schedule& schedule) const {
        std::vector<int> teacherHours(numTeachers, 0);
        for (const auto& timeslot : schedule.timetable) {
            if (timeslot.teacherIdx != -1) {
                teacherHours[timeslot.teacherIdx]++;
                if (teacherHours[timeslot.teacherIdx] > teachers[timeslot.teacherIdx].maxHours) {
                    return false;
                }
            }
        }
        return true;
    }

    bool noGroupOverload(const Schedule& schedule) const {
        std::vector<int> groupHours(numGroups, 0);
        for (const auto& timeslot : schedule.timetable) {
            if (timeslot.groupIdx != -1) {
                groupHours[timeslot.groupIdx]++;
                if (groupHours[timeslot.groupIdx] > groups[timeslot.groupIdx].maxHours) {
                    return false;
                }
            }
        }
        return true;
    }

    void backtracking(size_t timeslotIdx) {
        if (timeslotIdx == numTimeslots) {
            bestSchedule = schedule;
            return;
        }

        std::vector<int> variableOrder = getVariableOrder(timeslotIdx);

        for (int assignment : variableOrder) {
            schedule.timetable[timeslotIdx].subjectIdx = assignment % numSubjects;
            schedule.timetable[timeslotIdx].teacherIdx = (assignment / numSubjects) % numTeachers;
            schedule.timetable[timeslotIdx].groupIdx = assignment / (numSubjects * numTeachers);

            if (constraintsSatisfied(schedule) && isValidAssignment(timeslotIdx,
                                                                   schedule.timetable[timeslotIdx].subjectIdx,
                                                                   schedule.timetable[timeslotIdx].teacherIdx,
                                                                   schedule.timetable[timeslotIdx].groupIdx)) {
                backtracking(timeslotIdx + 1);
            }
        }
    }

    std::vector<int> getVariableOrder(size_t timeslotIdx) {
        std::vector<int> values = domains[timeslotIdx];

        std::sort(values.begin(), values.end(), [this](const auto& lhs, const auto& rhs) {
            int subjectLhs = lhs % numSubjects;
            int subjectRhs = rhs % numSubjects;
            int numTeachersLhs = countTeachersForSubject(subjectLhs);
            int numTeachersRhs = countTeachersForSubject(subjectRhs);
            return numTeachersLhs < numTeachersRhs;
        });

        return values;
    }

    int countTeachersForSubject(int subjectIdx) {
        int count = 0;

        for (const auto& teacher : teachers) {
            if (teacher.subjectKnowledge[subjectIdx]) {
                ++count;
            }
        }

        return count;
    }

    bool constraintsSatisfied(const Schedule& schedule) const {
        for (const auto& constraint : constraints) {
            if (!constraint(schedule)) {
                return false;
            }
        }
        return true;
    }
};

int main() {
    CSPSolver csp;
    csp.solve();
    csp.printSchedule(csp.getBestSchedule());
}
