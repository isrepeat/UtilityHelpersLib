using net.r_eg.MvsSln;
using net.r_eg.MvsSln.Core;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SlnInjectorWPF
{
    public class Solution
    {
        private Sln sln;
        private const SlnItems DefaultItems =
            SlnItems.Projects
            | SlnItems.SolutionItems
            | SlnItems.ProjectDependenciesXml
            | SlnItems.ProjectDependencies;

        public Solution(string path, SlnItems itemsToLoad = DefaultItems) {
            sln = new Sln(path, itemsToLoad);

            foreach(var projItem in Items.ProjectItems)
            {
                Projects.Add(new Project(projItem));
            }
        }

        public ISlnResult Items { get { return sln.Result; } }

        public IList<Project> Projects { get; } = new List<Project>();
    }
}
