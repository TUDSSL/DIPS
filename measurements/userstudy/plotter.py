import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import StrMethodFormatter
import csv

category_names_3 = ['EDB', 'Both', 'DIPS', 'Neither']
results_2 = {
    'I would use it': [],
    'I am familiar with it': [],
    'Intuitive to use': [],
    'Easy to use': [],
}

category_names_1 = ['Zero', 'One', 'Two', 'Three']
results_1 = {
    'Bugs found with EDB': [],
    'Bugs found with DIPS': []
}

familiar_ranking = {
    "Very different from what I am used to": -2,
    "Different from what I am used to": -1,
    "Neither different nor similar": 0,
    "Similar to what I am used to": 1,
    "Very similar to what I am used to": 2
}

intuitive_ranking = {
    "Absolutely non-intuitive": -2,
    "Non-intuitive": -1,
    "Neither intuitive nor non-intuitive": 0,
    "Intuitive": 1,
    "Absolutely intuitive": 2,
}

easy_ranking = {
    "Very difficult": -2,
    "Somewhat difficult": -1,
    "Neither easy nor difficult": 0,
    "Easy": 1,
    "Very easy": 2,
}

use_ranking = {
    "No": -1,
    "Yes": 1,
}


def percentage(array, participants):
    for n in range(4):
        array[n] = array[n] / participants * 100

    return array


def get_ranking(row, columnID, ranking):
    print(row[columnID])
    if (ranking[row[columnID]] == ranking[row[columnID + 1]]) <= 0:
        return 3
    if (ranking[row[columnID]] == ranking[row[columnID + 1]]) > 0:
        return 1
    if ranking[row[columnID]] > ranking[row[columnID + 1]]:
        return 2
    if ranking[row[columnID]] < ranking[row[columnID + 1]]:
        return 0


def get_data(file, studyID):
    plt.rcParams.update({
        "text.usetex": True,
    })
    plt.rcParams.update({'ytick.minor.visible': False})
    #
    fig_1, ax0 = plt.subplots(figsize=(7, 1.3), tight_layout=True)
    fig_2, ax1 = plt.subplots(figsize=(7, 2), tight_layout=True)

    with open(file) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        line_count = 0

        bugs_edb = [0, 0, 0, 0]
        bugs_dips = [0, 0, 0, 0]

        use = [0, 0, 0, 0]
        familiar = [0, 0, 0, 0]
        intuitive = [0, 0, 0, 0]
        easy = [0, 0, 0, 0]
        participants = 0

        for row in csv_reader:
            if line_count != 0 and len(row) != 0:
                if row[1] != '':  # check if survey is finished
                    participants += 1
                    # Check the found bugs
                    dips = 1 if row[10] == 'Yes' else 0
                    dips += 1 if row[15] == 'Yes' else 0
                    dips += 1 if row[20] == 'Yes' else 0
                    bugs_dips[dips] += 1

                    edb = 1 if row[26] == 'Yes' else 0
                    edb += 1 if row[31] == 'Yes' else 0
                    edb += 1 if row[36] == 'Yes' else 0
                    bugs_edb[edb] += 1

                    use[get_ranking(row, 39, use_ranking)] += 1
                    familiar[get_ranking(row, 41, familiar_ranking)] += 1
                    intuitive[get_ranking(row, 43, intuitive_ranking)] += 1
                    easy[get_ranking(row, 45, easy_ranking)] += 1

            line_count += 1

        # now make percentage
        results_1['Bugs found with EDB'] = percentage(bugs_edb, participants)
        results_1['Bugs found with DIPS'] = percentage(bugs_dips, participants)

        results_2['I would use it'] = percentage(use, participants)
        results_2['I am familiar with it'] = percentage(familiar, participants)
        results_2['Intuitive to use'] = percentage(intuitive, participants)
        results_2['Easy to use'] = percentage(easy, participants)

        survey(ax0, results_1, category_names_1)
        survey(ax1, results_2, category_names_3)

        fig_1.savefig(f'userstudy{studyID}_results_1.pdf', bbox_inches='tight', pad_inches=0.05)
        fig_2.savefig(f'userstudy{studyID}_results_2.pdf', bbox_inches='tight', pad_inches=0.05)


def survey(ax, results, category_names):
    """
    Parameters
    ----------
    results : dict
        A mapping from question labels to a list of answers per category.
        It is assumed all lists contain the same number of entries and that
        it matches the length of *category_names*.
    category_names : list of str
        The category labels.
    """
    labels = list(results.keys())
    data = np.array(list(results.values()))
    data_cum = data.cumsum(axis=1)
    category_colors = plt.colormaps['RdYlGn'](
        np.linspace(0.15, 0.85, data.shape[1]))

    ax.invert_yaxis()
    ax.xaxis.set_visible(False)
    data_max = np.sum(data, axis=1).max()
    ax.set_xlim(0, data_max)

    for i, (colname, color) in enumerate(zip(category_names, category_colors)):
        widths = data[:, i]
        starts = data_cum[:, i] - widths
        rects = ax.barh(labels, widths, left=starts, height=.8,
                        label=colname, color=color)
        xcenters = starts + widths / 2
        f_labels = list(results.values())

        for y, (x, c) in enumerate(zip(xcenters, widths)):
            fontsize = 7
            if c <= 0:
                marker = str()
            else:
                if c < data_max * .02:
                    fontsize = 7
                marker = "\\textbf{" + str(float(round(f_labels[y][i], 1))) + "\%}"
            ax.text(x, y, marker, ha='center', va='center', fontsize=fontsize)

    ax.legend(ncol=len(category_names), bbox_to_anchor=(.965, 1), loc='lower right', fontsize=11)

    labels = ax.get_yticklabels()
    plt.setp(labels, fontsize=11)


get_data("study2.csv", 2)
get_data("study1.csv", 1)
